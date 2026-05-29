#pragma once
#include "elm327.hpp"
#include <string>
#include <vector>
#include <map>
#include <cstdint> 
#include <cstddef> 


// Estructura para datos GM
struct GMData {
    std::string command;
    std::string response;
    bool success;
    std::string errorMsg;
};

// Clase para manejar comandos específicos de GM
class GMCommands {
public:
    explicit GMCommands(ELM327* elm);
    ~GMCommands();

    // Comandos básicos GM
    std::string getKilometers();        // F190 - Kilometraje ECU
    std::string getCatalystTemp();      // F40C - Temperatura catalizador
    std::string getFuelPressure();      // F423 - Presión combustible
    std::string getEngineTorque();      // F40D - Torque motor
    std::string getECUVoltage();        // F40A - Voltaje ECU
    std::string getGMHistory();         // 19 02 - Historial errores
    std::string clearGMHistory();       // 14 FF FF - Borrar historial
    std::string resetAdaptations();     // 22 F10E - Reset adaptativos (Chevrolet/GM)
    std::string resetAdaptations2();    // 22 F10D - Reset adaptativos método 2
    std::string resetFuelTrims();       // 22 F11C / F10B / F117 - Reset Fuel Trims
    std::string resetAllAdaptations();  // Ejecuta todos los métodos de reset
    
    // Nuevas lecturas GM
    std::string getTransmissionTemp();  // 22 1940 - Temperatura transmisión
    std::string getOilPressure();       // 22 1470 - Presión aceite motor
    std::string getKnockRetard();       // 22 11A2 - Knock retard
    
    // Escaneo de PIDs
    void scanPIDs(uint16_t start = 0xF400, uint16_t end = 0xF4FF);
    
    // Comando personalizado
    std::string sendCommand(const std::string& pidHex, bool useGMHeader = true);
    
    // Decodificar respuestas
    std::string decodeKilometers(const std::string& hexResponse);
    std::string decodeCatalystTemp(const std::string& hexResponse);
    std::string decodeFuelPressure(const std::string& hexResponse);
    std::string decodeEngineTorque(const std::string& hexResponse);
    std::string decodeECUVoltage(const std::string& hexResponse);
    std::string decodeGMHistory(const std::string& hexResponse);
    std::string decodeTransmissionTemp(const std::string& hexResponse);
    std::string decodeOilPressure(const std::string& hexResponse);
    std::string decodeKnockRetard(const std::string& hexResponse);
    
private:
    ELM327* elm;
    std::string lastResponse;
    
    // Configurar cabezal para GM
    bool setupGMHeader();
    bool restoreHeader();
    
    // Guardar cabezales originales
    std::string oldHeader;
    std::string oldCRA;
    
    // Procesar respuesta
    std::string processResponse(const std::string& response, const std::string& expectedPrefix = "62");
    
    // Mostrar error
    void showError(const std::string& cmd, const std::string& response);
};
