#ifndef CNCCONFIG_H
#define CNCCONFIG_H

/**
 * @brief Namespace contendo configurações fixas de hardware e software para a CNC.
 */
namespace CncConfig {
    /** @brief Limite máximo no eixo X em milímetros. */
    const double X_MAX = 2.0; 
    /** @brief Limite máximo no eixo Y em milímetros. */
    const double Y_MAX = 2.0;
    /** @brief Limite máximo no eixo Z em milímetros. */
    const double Z_MAX = 2.0;
    
    /** @brief Limite mínimo no eixo X em milímetros. */
    const double X_MIN = -200.0;
    /** @brief Limite mínimo no eixo Y em milímetros. */
    const double Y_MIN = -160.0;
    /** @brief Limite mínimo no eixo Z em milímetros. */
    const double Z_MIN = -18.0;

    /** @brief Velocidade de avanço padrão (Feed Rate) em mm/min. */
    const int DEFAULT_FEED_RATE = 300;
    
    /** @brief Intervalo de consulta de status (polling) em milissegundos. */
    const int STATUS_POLLING_MS = 100;

    /** @brief Velocidade de transmissão da porta serial. */
    const int BAUD_RATE = 115200;
}

#endif // CNCCONFIG_H