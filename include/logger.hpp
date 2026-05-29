#pragma once

#include <fstream>
#include <memory>
#include <string>

/**
 * @brief Clase para registrar datos en un archivo CSV.
 * 
 * Permite abrir un archivo de log con nombre basado en la fecha/hora,
 * y escribir pares clave-valor junto con una marca de tiempo Unix.
 * Los datos se guardan en formato CSV con columnas: timestamp, key, value.
 */
class Logger
{
private:
    std::unique_ptr<std::ofstream> file; ///< Manejador del archivo de salida.
    std::string filename;                ///< Nombre del archivo de log generado.

public:
    /**
     * @brief Constructor. No abre el archivo automáticamente; debe llamarse a open().
     */
    Logger();

    /**
     * @brief Destructor. Cierra el archivo si estaba abierto.
     */
    ~Logger();

    /**
     * @brief Abre un nuevo archivo de log con nombre auto-generado.
     * 
     * El nombre sigue el patrón: log_YYYYMMDD_HHMM.csv
     * La primera línea del archivo contiene los encabezados: timestamp,key,value
     * 
     * @return true si el archivo se abrió correctamente, false en caso contrario.
     */
    bool open();

    /**
     * @brief Registra un par clave-valor como cadena de texto.
     * 
     * @param key   Identificador del dato (ej. "RPM", "Speed").
     * @param value Valor en formato string.
     */
    void log(const std::string& key, const std::string& value);

    /**
     * @brief Registra un par clave-valor como entero.
     * 
     * @param key   Identificador del dato.
     * @param value Valor entero.
     */
    void log(const std::string& key, int value);

    /**
     * @brief Registra un par clave-valor como flotante.
     * 
     * @param key   Identificador del dato.
     * @param value Valor flotante.
     */
    void log(const std::string& key, float value);

    /**
     * @brief Registra un par clave-valor como double.
     * 
     * @param key   Identificador del dato.
     * @param value Valor double.
     */
    void log(const std::string& key, double value);

    /**
     * @brief Cierra el archivo de log si está abierto.
     * 
     * También vacía el buffer y libera los recursos.
     */
    void close();
};