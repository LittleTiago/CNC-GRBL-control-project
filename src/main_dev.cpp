#include <QCoreApplication>
#include <QSocketNotifier>
#include <QTextStream>
#include <iostream>
#include "CncService.h"

/**
 * @brief Main de Desenvolvimento: Escuta comandos via STDIN (Pipe do Python).
 */
int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    CncService cnc;

    /* *********** Configuração de Logs para Debug ************ */
    QObject::connect(&cnc, &CncService::logUpdated, [](const CncLog &log){
        fprintf(stderr, "[%s] %s | RES: %s | RESULT: %d\n", 
                log.timestamp.toUtf8().constData(),
                log.commandSent.toUtf8().constData(),
                log.rawResponse.toUtf8().constData(),
                (int)log.result);
    });

    /* *********** Monitor de Entrada (Vinculado ao Python) ************ */
    QSocketNotifier notifier(fileno(stdin), QSocketNotifier::Read);
    QObject::connect(&notifier, &QSocketNotifier::activated, [&](){
        QTextStream in(stdin);
        QString line = in.readLine();
        if (line.isEmpty()) return;

        if (line.startsWith("REL")) {
            QStringList p = line.split(" ");
            cnc.moveRelative(p[1].toDouble(), p[2].toDouble(), p[3].toDouble());
        } else if (line == "HOME") {
            cnc.executeHoming();
        } else if (line == "RESET") {
            cnc.executeReset();
        }
    });

    // Conecta na porta passada por argumento ou default
    QString porta = (argc > 1) ? argv[1] : "/dev/ttyUSB1";
    cnc.connectToMachine(porta);

    return a.exec();
}