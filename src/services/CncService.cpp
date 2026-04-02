#include "CncService.h"
#include <QDebug>

CncService::CncService(QObject *parent) : QObject(parent)
{
    m_serial = new QSerialPort(this);
    m_statusTimer = new QTimer(this);

    // Forçamos o uso da assinatura de ponteiro de função do QObject
    QObject::connect(m_statusTimer, &QTimer::timeout, 
                     this, &CncService::requestStatus);
    
    QObject::connect(m_serial, &QSerialPort::readyRead, 
                     this, &CncService::handleReadyRead);
}

CncService::~CncService()
{
    disconnect();
}

bool CncService::connect(const QString &portName, int baudRate)
{
    m_serial->setPortName(portName);
    m_serial->setBaudRate(baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);

    if (m_serial->open(QIODevice::ReadWrite)) {
        m_statusTimer->start(200); // Começa a pedir status
        qDebug() << "CNC Conectada em" << portName;
        return true;
    }
    
    emit errorOccurred("Falha ao abrir porta serial.");
    return false;
}

void CncService::disconnect()
{
    if (m_serial->isOpen()) {
        m_statusTimer->stop();
        m_serial->close();
        m_machineState = "Disconnected";
        emit stateChanged(m_machineState);
    }
}

bool CncService::isConnected() const
{
    return m_serial->isOpen();
}

void CncService::requestStatus()
{
    if (m_serial->isOpen()) {
        m_serial->write("?"); // Pergunta o estado em tempo real
    }
}

void CncService::handleReadyRead()
{
    // Lê tudo que chegou, limpando espaços
    QByteArray rawData = m_serial->readAll();
    QString data = QString::fromUtf8(rawData).trimmed();

    // Se a resposta for de status (entre < >), processa
    if (data.startsWith("<")) {
        processRawData(data);
    }
}

void CncService::processRawData(QString data)
{
    // Exemplo: <Idle|WPos:10.000,5.000,0.000|Bf:15,128>
    
    // 1. Extrair Estado (Cor do LED)
    QString state = data.mid(1, data.indexOf('|') - 1);
    if (state != m_machineState) {
        m_machineState = state;
        emit stateChanged(m_machineState);
    }

    // 2. Extrair Coordenadas (DRO)
    int wposIndex = data.indexOf("WPos:");
    if (wposIndex != -1) {
        // Pega a string após WPos: até o próximo |
        QString coordsPart = data.mid(wposIndex + 5).split('|').first();
        QStringList xyz = coordsPart.split(',');

        if (xyz.size() >= 3) {
            m_currentX = xyz[0].toDouble();
            m_currentY = xyz[1].toDouble();
            m_currentZ = xyz[2].toDouble();
            emit positionChanged(m_currentX, m_currentY, m_currentZ);
        }
    }
}

void CncService::moveAxis(QString axis, double step, int speed) {
    if (!m_serial->isOpen()) return;

    // Trava de segurança (máximo 300 mm/min)
    int safeSpeed = (speed > 300) ? 300 : speed;

    // G91 = Incremental (anda X em relação a onde está agora)
    QString cmd = QString("G91 G1 %1%2 F%3\r\n").arg(axis).arg(step).arg(safeSpeed);
    m_serial->write(cmd.toUtf8());
}

void CncService::stopMovement() {
    if (m_serial->isOpen()) {
        m_serial->write("!"); // Comando de parada imediata (Feed Hold) do GRBL
    }
}

void CncService::sendCommand(const QString &command) {
    if (m_serial->isOpen()) {
        QString fullCmd = command.trimmed() + "\r\n";
        m_serial->write(fullCmd.toUtf8());
    }
}