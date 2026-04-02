#ifndef CNCSERVICE_H
#define CNCSERVICE_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QStringList>

class CncService : public QObject
{
    Q_OBJECT

public:
    explicit CncService(QObject *parent = nullptr);
    ~CncService();

    // Conexão
    bool connect(const QString &portName, int baudRate = 115200);
    void disconnect();

    // Comandos de Movimento 
    void moveAxis(QString axis, double step, int speed);
    void stopMovement();
    void sendCommand(const QString &command);

    // Getters para a UI
    double getX() const { return m_currentX; }
    double getY() const { return m_currentY; }
    double getZ() const { return m_currentZ; }
    QString getState() const { return m_machineState; }
    bool isConnected() const;

signals:
    // Sinais que a (UI) vai "escutar"
    void positionChanged(double x, double y, double z);
    void stateChanged(QString newState);
    void errorOccurred(QString error);

private slots:
    void handleReadyRead();
    void requestStatus(); // request através do'?'

private:
    void processRawData(QString data);

    QSerialPort *m_serial;
    QTimer *m_statusTimer;

    double m_currentX = 0.0;
    double m_currentY = 0.0;
    double m_currentZ = 0.0;
    QString m_machineState = "Disconnected";
};

#endif // CNCSERVICE_H