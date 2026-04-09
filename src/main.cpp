#include <QCoreApplication>
#include <QDebug>

/**
 * @brief Main de Produção (Vazia para desenvolvimento).
 * Este arquivo será populado quando a interface gráfica for implementada.
 */
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qDebug() << "Openscience: Executável principal em modo de espera.";
    qDebug() << "Utilize o 'cnc_dev' para testes com o simulador Python.";

    // Retorna o loop de eventos básico, mas sem nenhuma lógica ativa.
    return a.exec();
}