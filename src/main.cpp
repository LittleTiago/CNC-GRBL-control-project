#include <QCoreApplication>
#include <QDebug>
#include "CncService.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    CncService cnc;

    // 1. ESCUTAR A POSIÇÃO: Sempre que a CNC responder o '?', este bloco roda
    QObject::connect(&cnc, &CncService::positionChanged, [](double x, double y, double z){
        qDebug() << "Posição Atual -> [ X:" << x << "| Y:" << y << "| Z:" << z << "]";
    });

    // 2. ESCUTAR O ESTADO: Verde/Amarelo/Vermelho
    QObject::connect(&cnc, &CncService::stateChanged, [](QString state){
        qDebug() << "ESTADO DA MÁQUINA MUDOU PARA:" << state;
    });

    // 3. TENTAR CONECTAR
    // IMPORTANTE: Se você estiver no Windows, mude para "COM3", "COM4", etc.
    // No Linux, geralmente é "/dev/ttyUSB0" ou "/dev/ttyACM0"
    QString porta = "/dev/ttyUSB1"; 
    
    qDebug() << "Tentando abrir a porta:" << porta;

    if (cnc.connect(porta, 115200)) {
        qDebug() << "CONECTADO COM SUCESSO!";
        
        // Teste de Movimento: Move 10mm no X a 300mm/min após 2 segundos
        // Isso é só para você ver os números mudando no terminal
        QTimer::singleShot(2000, [&](){
            qDebug() << "Enviando comando de teste: Mover X 10mm";
            cnc.moveAxis("X", 10.0, 300);
        });

    } else {
        qDebug() << "ERRO: Não foi possível abrir a porta." << porta;
        qDebug() << "DICA: Verifique se o cabo está plugado ou use 'sudo chmod 666" << porta << "'";
    }

    return a.exec();
}