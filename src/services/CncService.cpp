#include "CncService.h"
#include <QDebug>

/**
 * @brief Construtor da classe CncService.
 * Inicializa o timer de polling e conecta os sinais internos de comunicação.
 */
CncService::CncService(QObject *parent) : QObject(parent) {
    m_statusTimer = new QTimer(this);
    
    /* *********** Conexões de Sinais e Slots Internos ************ */
    connect(m_statusTimer, &QTimer::timeout, this, &CncService::onPollStatus);
    connect(&m_serial, &QSerialPort::readyRead, this, &CncService::onReadyRead);
}

/**
 * @brief Destrutor da classe.
 * Garante que a conexão seja encerrada de forma limpa ao destruir o objeto.
 */
CncService::~CncService() {
    disconnectMachine();
}

/**
 * @brief Tenta abrir a comunicação com a porta serial especificada.
 * No Linux (Ubuntu), o parâmetro geralmente segue o padrão /dev/ttyUSBX.
 */
bool CncService::connectToMachine(const QString &portName) {
    m_serial.setPortName(portName);
    m_serial.setBaudRate(CncConfig::BAUD_RATE); // 115200
    
    /* ************ Tentativa de Abertura do Hardware ************ */
    if (m_serial.open(QIODevice::ReadWrite)) {
        m_statusTimer->start(CncConfig::STATUS_POLLING_MS); // 100ms
        addToLog("SYSTEM", "Porta aberta com sucesso em " + portName, CommandResult::Success);
        return true;
    }
    
    addToLog("SYSTEM", "Falha ao abrir porta: " + portName, CommandResult::Disconnected);
    return false;
}

/**
 * @brief Encerra a conexão e para o monitoramento de status.
 */
void CncService::disconnectMachine() {
    if (m_serial.isOpen()) {
        m_statusTimer->stop();
        m_serial.close();
        addToLog("SYSTEM", "Conexão encerrada pelo usuário", CommandResult::Success);
    }
}

/**
 * @brief Valida se o alvo está dentro da área de trabalho permitida (150x150x50mm).
 */
bool CncService::checkLimits(double x, double y, double z) {
    
    if (x < CncConfig::X_MIN || x > CncConfig::X_MAX) return false;
    if (y < CncConfig::Y_MIN || y > CncConfig::Y_MAX) return false;
    if (z < CncConfig::Z_MIN || z > CncConfig::Z_MAX) return false;
    return true;
}

/**
 * @brief Executa movimento para coordenada específica (G90).
 */
CommandResult CncService::moveAbsolute(double x, double y, double z) {

    /* *********** Verificações de Conexão, Estado e Limites ************ */
    if (!m_serial.isOpen()) return CommandResult::Disconnected;
    if (m_currentState == MachineState::Alarm) return CommandResult::MachineAlarmed;
    
    // Bloqueia se a CNC ainda estiver processando movimento anterior
    if (m_currentState != MachineState::Idle) {
        addToLog("G90...", "Rejeitado: Máquina Ocupada", CommandResult::MachineBusy);
        return CommandResult::MachineBusy;
    }
    
    if (!checkLimits(x, y, z)) {
        addToLog(QString("G90 X%1...").arg(x), "Bloqueado: Limite atingido", CommandResult::OutOfSoftLimits);
        return CommandResult::OutOfSoftLimits;
    }
    
    /* ************ Construção do Comando e Envio *************************** */
    QString cmd = QString("G90 G1 X%1 Y%2 Z%3 F%4\r\n")
                  .arg(x, 0, 'f', 3)
                  .arg(y, 0, 'f', 3)
                  .arg(z, 0, 'f', 3)
                  .arg(CncConfig::DEFAULT_FEED_RATE);

    m_serial.write(cmd.toUtf8());
    addToLog(cmd.trimmed(), "Enviado ao buffer", CommandResult::Success);
    return CommandResult::Success;
}

/**
 * @brief Executa movimento a partir da posição atual (G91).
 */
CommandResult CncService::moveRelative(double dx, double dy, double dz) {

    /* *********** Validações antes do Envio ************ */
    if (!m_serial.isOpen()) return CommandResult::Disconnected;
    if (m_currentState == MachineState::Alarm) return CommandResult::MachineAlarmed;
    if (m_currentState != MachineState::Idle) {
        addToLog("G91...", "Rejeitado: Máquina Ocupada", CommandResult::MachineBusy);
        return CommandResult::MachineBusy;
    }

    // Calcula posição futura baseada no feedback em tempo real
    double targetX = m_posX + dx;
    double targetY = m_posY + dy;
    double targetZ = m_posZ + dz;

    if (!checkLimits(targetX, targetY, targetZ)) {
        addToLog(QString("G91 X%1...").arg(dx), "Bloqueado: Fora dos limites", CommandResult::OutOfSoftLimits);
        return CommandResult::OutOfSoftLimits;
    }

    /* ************ Envio do Comando Relativo *************************** */
    QString cmd = QString("G91 G1 X%1 Y%2 Z%3 F%4\r\n")
                  .arg(dx, 0, 'f', 3)
                  .arg(dy, 0, 'f', 3)
                  .arg(dz, 0, 'f', 3)
                  .arg(CncConfig::DEFAULT_FEED_RATE);

    m_serial.write(cmd.toUtf8());
    addToLog(cmd.trimmed(), "Comando Relativo Enviado", CommandResult::Success);
    return CommandResult::Success;
}

/**
 * @brief Inicia o ciclo de busca de referência da máquina ($H).
 */
CommandResult CncService::executeHoming() {
    if (!m_serial.isOpen()) return CommandResult::Disconnected;
    
    // Homing é permitido em Idle ou para destravar Alarmes
    if (m_currentState != MachineState::Idle && m_currentState != MachineState::Alarm) {
        addToLog("$H", "Rejeitado: Máquina Ocupada", CommandResult::MachineBusy);
        return CommandResult::MachineBusy;
    }

    m_serial.write("$H\r\n");
    addToLog("$H", "Iniciando Ciclo de Homing", CommandResult::Success);
    return CommandResult::Success;
}

/**
 * @brief Solicita periodicamente o status '?' para o firmware GRBL.
 */
void CncService::onPollStatus() {
    if (m_serial.isOpen() && m_serial.isWritable()) {
        m_serial.write("?");
    }
}

/**
 * @brief Lê e filtra as respostas brutas vindas da CNC.
 */
void CncService::onReadyRead() {
    /* ************ Leitura Agressiva do Buffer Serial ************ */
    m_serialBuffer.append(m_serial.readAll());

    /* ************ Processamento de Linhas Completas ************ */
    while (m_serialBuffer.contains('\n')) {
        int pos = m_serialBuffer.indexOf('\n');
        
        // Extrai a linha e remove caracteres de controle (\r, \n)
        QString line = QString::fromUtf8(m_serialBuffer.left(pos)).trimmed();
        m_serialBuffer.remove(0, pos + 1);

        if (line.isEmpty()) continue;

        // Registra qualquer resposta vinda da CNC no log do sistema
        addToLog("CNC_REPORT", line, CommandResult::Success);

        /* ************ Triagem de Respostas ************ */
        if (line.startsWith("<")) {
            parseStatusString(line);
        } else if (line == "ok") {
            // Confirmação de recebimento pelo firmware
        } else if (line.startsWith("error:") || line.startsWith("ALARM:")) {
            addToLog("CNC_ERROR", line, CommandResult::MachineAlarmed);
        }
    }
}

/**
 * @brief Analisa a string de status e emite sinais de posição e estado.
 */
void CncService::parseStatusString(const QString &status) {
    // Formato esperado: <State|WPos:X,Y,Z|Bf:B,R|FS:F,S>
    QString content = status.mid(1, status.length() - 2);
    QStringList parts = content.split("|");
    if (parts.isEmpty()) return;

    /* ************ 1. Detecção de Estado ************ */
    QString stateStr = parts.at(0);
    MachineState newState = MachineState::Unknown;
    
    if (stateStr.startsWith("Idle")) newState = MachineState::Idle;
    else if (stateStr.startsWith("Run")) newState = MachineState::Run;
    else if (stateStr.startsWith("Alarm")) newState = MachineState::Alarm;
    else if (stateStr.startsWith("Home")) newState = MachineState::Home;
    else if (stateStr.startsWith("Hold")) newState = MachineState::Hold;

    if (newState != m_currentState) {
        // Notifica o término de um movimento (Run -> Idle)
        if (m_currentState == MachineState::Run && newState == MachineState::Idle) {
            emit movementFinished();
        }
        m_currentState = newState;
        emit stateChanged(m_currentState);
    }

    /* ************ 2. Extração de Coordenadas ************ */
    for (const QString &part : parts) {
        if (part.startsWith("WPos:")) {
            QStringList coords = part.mid(5).split(",");
            if (coords.size() >= 3) {
                m_posX = coords[0].toDouble();
                m_posY = coords[1].toDouble();
                m_posZ = coords[2].toDouble();
                emit positionChanged(m_posX, m_posY, m_posZ);
            }
            break;
        }
    }
}

/**
 * @brief Envia o comando de Soft Reset (Ctrl+X / 0x18).
 * Este comando é prioritário e limpa o buffer do GRBL instantaneamente.
 */
void CncService::executeReset() {
    
    /* ************ Envio do Comando de Tempo Real (Hex 0x18) ************ */
    const char ctrlX = 0x18; 
    m_serial.write(&ctrlX, 1);
    
    addToLog("CTRL+X", "Comando de Soft Reset enviado", CommandResult::Success);
}

/**
 * @brief Envia o comando $X para destravar o alarme sem forçar um Homing.
 * Útil para recuperar o controle após exceder limites lógicos.
 */
CommandResult CncService::unlockAlarm() {
    if (!m_serial.isOpen()) return CommandResult::Disconnected;

    /* ************ Envio do Comando Kill Alarm Lock ******************* */
    m_serial.write("$X\r\n");
    addToLog("$X", "Solicitação de destravamento (Unlock) enviada", CommandResult::Success);
    
    return CommandResult::Success;
}

/**
 * @brief Centraliza a criação de registros de log para o sistema.
 */
void CncService::addToLog(const QString &cmd, const QString &raw, CommandResult res) {
    CncLog log;
    log.timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    log.commandSent = cmd;
    log.rawResponse = raw;
    log.result = res;
    
    emit logUpdated(log);
}