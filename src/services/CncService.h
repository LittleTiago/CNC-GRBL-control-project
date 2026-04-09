#ifndef CNCSERVICE_H
#define CNCSERVICE_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QDateTime>
#include "CncConfig.h"

/** @brief Estados operacionais mapeados diretamente do firmware GRBL. */
enum class MachineState { Idle, Run, Hold, Jog, Alarm, Home, Unknown };

/** @brief Resultados possíveis após tentativa de execução de um comando. */
enum class CommandResult { 
    Success,          /**< Comando validado e enviado com sucesso. */
    OutOfSoftLimits,  /**< Posição alvo excede os limites físicos definidos. */
    Disconnected,     /**< A porta serial não está aberta. */
    MachineAlarmed,   /**< CNC em estado de erro crítico (Alarm). */
    MachineBusy,      /**< Comando rejeitado pois a CNC ainda está em movimento. */
    InternalError     /**< Falha genérica no processamento do comando. */
};

/** @brief Estrutura para armazenamento e emissão de logs de transação. */
struct CncLog {
    QString timestamp;    /**< Horário exato da ocorrência. */
    QString commandSent;  /**< String G-Code enviada ou descrição do evento. */
    QString rawResponse;  /**< Resposta bruta recebida da CNC. */
    CommandResult result; /**< Status final do processamento. */
};

/**
 * @brief Classe responsável pela comunicação e controle lógico da CNC GRBL.
 */
class CncService : public QObject {
    Q_OBJECT

public:
    explicit CncService(QObject *parent = nullptr);
    ~CncService();

    /** @brief Tenta abrir a comunicação com a porta serial especificada. */
    bool connectToMachine(const QString &portName);

    /** @brief Encerra a conexão serial e para o timer de status. */
    void disconnectMachine();

    /** @brief Executa movimento para coordenada específica (G90). */
    CommandResult moveAbsolute(double x, double y, double z);

    /** @brief Executa movimento a partir da posição atual (G91). */
    CommandResult moveRelative(double dx, double dy, double dz);

    /** @brief Inicia o ciclo de busca de referência da máquina ($H). */
    CommandResult executeHoming();

    /** @brief Executa um Soft Reset no GRBL (Ctrl+X) para sair de estados de erro. */
    void executeReset();

    /** @brief Destrava o estado de alarme do GRBL enviando o comando $X. */
    CommandResult unlockAlarm();

signals:
    /** @brief Emitido quando a posição da CNC é atualizada. */
    void positionChanged(double x, double y, double z);
    /** @brief Emitido quando o estado da máquina (Idle, Run, Alarm) muda. */
    void stateChanged(MachineState newState);
    /** @brief Emitido para cada nova entrada de log gerada. */
    void logUpdated(const CncLog &log);
    /** @brief Sinal verde: indica que a CNC terminou o movimento e voltou a ficar Idle. */
    void movementFinished();

private slots:
    /** @brief Slot interno para processar dados recebidos da serial. */
    void onReadyRead();
    /** @brief Slot disparado pelo timer para solicitar status '?' à CNC. */
    void onPollStatus();

private:
    /** @brief Valida se o alvo está dentro da área de trabalho permitida. */
    bool checkLimits(double x, double y, double z);
    /** @brief Analisa a string de status do GRBL e extrai dados. */
    void parseStatusString(const QString &status);
    /** @brief Centraliza a criação de logs e emissão de sinais. */
    void addToLog(const QString &cmd, const QString &raw, CommandResult res);

    QSerialPort m_serial;   /**< Objeto de comunicação serial do Qt. */
    QTimer *m_statusTimer;  /**< Timer para requisição periódica de status. */
    QByteArray m_serialBuffer; /**< Buffer para acumular dados brutos da serial. */

    double m_posX = 0.0;    /**< Posição atual no eixo X. */
    double m_posY = 0.0;    /**< Posição atual no eixo Y. */
    double m_posZ = 0.0;    /**< Posição atual no eixo Z. */
    
    /** @brief Estado atual da máquina, inicializado como Unknown. */
    MachineState m_currentState = MachineState::Unknown;
};

#endif // CNCSERVICE_H