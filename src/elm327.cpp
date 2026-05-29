//elm327.cpp
#include "elm327.hpp"

#include <fstream>
#include <functional>
#include <ctime>

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <chrono>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>



ELM327::ELM327(const std::string& macAddr, int ch)
    : sock(-1), mac(macAddr), channel(ch) {}

ELM327::~ELM327()
{
    disconnect();
}

//bool ELM327::isConnected() {
//    return sock >= 0;
//}

bool ELM327::connectBT()
{
    struct sockaddr_rc addr{};

    sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (sock < 0)
    {
        perror("[ERROR] socket");
        return false;
    }

    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t)channel;
    str2ba(mac.c_str(), &addr.rc_bdaddr);

    std::cout << "[INFO] Conectando a " << mac << "...\n";

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("[ERROR] connect");
        return false;
    }

    std::cout << "[OK] Conectado!\n";

    // Inicializar ELM327
    send("ATZ", 1000);
    send("ATE0"); // echo off
    send("ATL0"); // linefeed off
    send("ATS0"); // spaces off
    send("ATSP0"); // protocolo automático
    
    // Configurar para respuestas más rápidas
    send("ATAT1"); // adaptor timeout 1
    send("ATST20"); // timeout 200ms

    return true;
}

void ELM327::disconnect()
{
    if (sock >= 0)
    {
        std::cout << "[INFO] Cerrando conexión\n";
        close(sock);
        sock = -1;
    }
}

std::string ELM327::readRaw()
{
    if (!isConnected()) return "";
    
    char buffer[4096];
    int n = read(sock, buffer, sizeof(buffer)-1);

    if (n > 0)
    {
        buffer[n] = 0;
        std::string result(buffer);
        
        // Limpiar caracteres no deseados
        result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
        result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
        result.erase(std::remove(result.begin(), result.end(), ' '), result.end());
        
        return result;
    }

    return "";
}

std::string ELM327::send(const std::string& cmd, int delayMs)
{
    if (!isConnected()) return "";
    
    std::string full = cmd + "\r";

    std::cout << "[TX] " << cmd << std::endl;

    write(sock, full.c_str(), full.size());
    usleep(delayMs * 1000);

    std::string resp = readRaw();

    if (!resp.empty()) {
        std::cout << "[RX] " << resp.substr(0, 100) << std::endl;
    }

    return resp;
}

std::vector<std::string> ELM327::splitResponse(const std::string& response) {
    std::vector<std::string> results;
    std::stringstream ss(response);
    std::string item;
    
    // Buscar el inicio de los datos OBD (después del comando echo)
    size_t dataStart = response.find_first_of("0123456789ABCDEF");
    if (dataStart != std::string::npos) {
        std::string data = response.substr(dataStart);
        
        // Separar por '>' si existe
        size_t arrowPos = data.find('>');
        if (arrowPos != std::string::npos) {
            data = data.substr(0, arrowPos);
        }
        
        // Dividir en bytes de 2 caracteres
        for (size_t i = 0; i < data.length(); i += 2) {
            if (i + 2 <= data.length()) {
                results.push_back(data.substr(i, 2));
            }
        }
    }
    
    return results;
}

// ================= IMPLEMENTACIÓN DE PARÁMETROS =================

int ELM327::getRPM()
{
    std::string r = send("010C");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 4) {  // RPM usa 2 bytes
        try {
            int a = std::stoi(bytes[2], nullptr, 16);
            int b = std::stoi(bytes[3], nullptr, 16);
            return (a * 256 + b) / 4;
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

int ELM327::getSpeed()
{
    std::string r = send("010D");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 3) {
        try {
            return std::stoi(bytes[2], nullptr, 16);
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

int ELM327::getCoolantTemp()
{
    std::string r = send("0105");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 3) {
        try {
            int temp = std::stoi(bytes[2], nullptr, 16);
            return temp - 40;
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

int ELM327::getTemp()
{
    return getCoolantTemp();
}

int ELM327::getEngineLoad()
{
    std::string r = send("0104");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 3) {
        try {
            int load = std::stoi(bytes[2], nullptr, 16);
            return (load * 100) / 255;
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

double ELM327::getThrottlePosition()
{
    std::string r = send("0111");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 3) {
        try {
            int pos = std::stoi(bytes[2], nullptr, 16);
            return (pos * 100.0) / 255.0;
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

double ELM327::getIntakePressure()
{
    std::string r = send("010B");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 3) {
        try {
            return std::stoi(bytes[2], nullptr, 16);
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

int ELM327::getIntakeTemp()
{
    std::string r = send("010F");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 3) {
        try {
            int temp = std::stoi(bytes[2], nullptr, 16);
            return temp - 40;
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

double ELM327::getTimingAdvance()
{
    std::string r = send("010E");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 3) {
        try {
            int advance = std::stoi(bytes[2], nullptr, 16);
            return advance / 2.0 - 64;
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

double ELM327::getFuelPressure()
{
    std::string r = send("010A");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 3) {
        try {
            int pressure = std::stoi(bytes[2], nullptr, 16);
            return pressure * 3;
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

double ELM327::getMAF()
{
    std::string r = send("0110");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 4) {
        try {
            int a = std::stoi(bytes[2], nullptr, 16);
            int b = std::stoi(bytes[3], nullptr, 16);
            return (a * 256 + b) / 100.0;
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

double ELM327::getFuelLevel()
{
    std::string r = send("012F");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 3) {
        try {
            int level = std::stoi(bytes[2], nullptr, 16);
            return (level * 100.0) / 255.0;
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

double ELM327::getAmbientTemp()
{
    std::string r = send("0146");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 3) {
        try {
            int temp = std::stoi(bytes[2], nullptr, 16);
            return temp - 40;
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

double ELM327::getOilTemp()
{
    std::string r = send("015C");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 3) {
        try {
            int temp = std::stoi(bytes[2], nullptr, 16);
            return temp - 40;
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

double ELM327::getCommandedEGR()
{
    std::string r = send("012C");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 3) {
        try {
            int egr = std::stoi(bytes[2], nullptr, 16);
            return (egr * 100.0) / 255.0;
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

double ELM327::getEGRError()
{
    std::string r = send("012D");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 3) {
        try {
            int error = std::stoi(bytes[2], nullptr, 16);
            return (error * 100.0) / 128.0 - 100;
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

double ELM327::getEVAPPressure()
{
    std::string r = send("0132");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 4) {
        try {
            int a = std::stoi(bytes[2], nullptr, 16);
            int b = std::stoi(bytes[3], nullptr, 16);
            return (a * 256 + b) / 4.0;
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

double ELM327::getBarometricPressure()
{
    std::string r = send("0133");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 3) {
        try {
            return std::stoi(bytes[2], nullptr, 16);
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

// ================= SENSORES DE OXÍGENO =================

std::vector<OxygenSensor> ELM327::getOxygenSensors()
{
    std::vector<OxygenSensor> sensors;
    
    // PID 0114 - Sensores de oxígeno (Banco 1, Sensores 1-4)
    std::string r1 = send("0114");
    auto bytes1 = splitResponse(r1);
    
    // Respuesta esperada: 41 14 [voltaje1] [trim1] [voltaje2] [trim2] [voltaje3] [trim3] [voltaje4] [trim4]
    if (bytes1.size() >= 4 && bytes1[0] == "41" && bytes1[1] == "14") {
        for (size_t i = 2; i + 1 < bytes1.size() && i < 10; i += 2) {
            try {
                int voltageByte = std::stoi(bytes1[i], nullptr, 16);
                int trimByte = std::stoi(bytes1[i+1], nullptr, 16);
                
                // Si ambos son 0xFF o 0x00, no hay sensor
                if ((voltageByte == 0xFF && trimByte == 0xFF) || 
                    (voltageByte == 0x00 && trimByte == 0x00)) {
                    continue;
                }
                
                OxygenSensor sensor;
                sensor.bank = 1;
                sensor.sensor = static_cast<int>((i - 2) / 2 + 1);
                
                // El voltaje para PID 0114 está en 0-1.275V (0 = 0V, 255 = 1.275V)
                sensor.voltage = voltageByte / 200.0;
                
                // Short term trim: 0-100% (0 = -100%, 128 = 0%, 255 = +99.2%)
                sensor.shortTermTrim = (trimByte * 100.0 / 128.0) - 100;
                
                sensors.push_back(sensor);
            } catch (...) {}
        }
    }
    
    // PID 0115 - Sensores de oxígeno (Banco 2, Sensores 1-4)
    std::string r2 = send("0115");
    auto bytes2 = splitResponse(r2);
    
    if (bytes2.size() >= 4 && bytes2[0] == "41" && bytes2[1] == "15") {
        for (size_t i = 2; i + 1 < bytes2.size() && i < 10; i += 2) {
            try {
                int voltageByte = std::stoi(bytes2[i], nullptr, 16);
                int trimByte = std::stoi(bytes2[i+1], nullptr, 16);
                
                if ((voltageByte == 0xFF && trimByte == 0xFF) || 
                    (voltageByte == 0x00 && trimByte == 0x00)) {
                    continue;
                }
                
                OxygenSensor sensor;
                sensor.bank = 2;
                sensor.sensor = static_cast<int>((i - 2) / 2 + 1);
                sensor.voltage = voltageByte / 200.0;
                sensor.shortTermTrim = (trimByte * 100.0 / 128.0) - 100;
                sensors.push_back(sensor);
            } catch (...) {}
        }
    }
    
    // PID 0116 - Sensores de oxígeno (Banco 1, Sensores 5-8) - Para vehículos con más de 4 sensores
    std::string r3 = send("0116");
    auto bytes3 = splitResponse(r3);
    
    if (bytes3.size() >= 4 && bytes3[0] == "41" && bytes3[1] == "16") {
        for (size_t i = 2; i + 1 < bytes3.size() && i < 10; i += 2) {
            try {
                int voltageByte = std::stoi(bytes3[i], nullptr, 16);
                int trimByte = std::stoi(bytes3[i+1], nullptr, 16);
                
                if ((voltageByte == 0xFF && trimByte == 0xFF) || 
                    (voltageByte == 0x00 && trimByte == 0x00)) {
                    continue;
                }
                
                OxygenSensor sensor;
                sensor.bank = 1;
                sensor.sensor = static_cast<int>((i - 2) / 2 + 5); // Sensores 5-8
                sensor.voltage = voltageByte / 200.0;
                sensor.shortTermTrim = (trimByte * 100.0 / 128.0) - 100;
                sensors.push_back(sensor);
            } catch (...) {}
        }
    }
    
    // PID 0117 - Sensores de oxígeno (Banco 2, Sensores 5-8) - Para vehículos con más de 4 sensores
    std::string r4 = send("0117");
    auto bytes4 = splitResponse(r4);
    
    if (bytes4.size() >= 4 && bytes4[0] == "41" && bytes4[1] == "17") {
        for (size_t i = 2; i + 1 < bytes4.size() && i < 10; i += 2) {
            try {
                int voltageByte = std::stoi(bytes4[i], nullptr, 16);
                int trimByte = std::stoi(bytes4[i+1], nullptr, 16);
                
                if ((voltageByte == 0xFF && trimByte == 0xFF) || 
                    (voltageByte == 0x00 && trimByte == 0x00)) {
                    continue;
                }
                
                OxygenSensor sensor;
                sensor.bank = 2;
                sensor.sensor = static_cast<int>((i - 2) / 2 + 5); // Sensores 5-8
                sensor.voltage = voltageByte / 200.0;
                sensor.shortTermTrim = (trimByte * 100.0 / 128.0) - 100;
                sensors.push_back(sensor);
            } catch (...) {}
        }
    }
    
    // Si no se encontraron sensores, mostrar mensaje
    if (sensors.empty()) {
        std::cout << "[INFO] No se detectaron sensores de oxígeno" << std::endl;
    }
    
    return sensors;
}
/*
std::vector<OxygenSensor> ELM327::getOxygenSensors()
{
    std::vector<OxygenSensor> sensors;
    
    // Procesar banco 1
    std::string r1 = send("0114");
    auto bytes1 = splitResponse(r1);
    
    // Respuesta esperada: 41 14 [voltaje] [trim] ...
    if (bytes1.size() >= 4 && bytes1[0] == "41" && bytes1[1] == "14") {
        // Procesar sensores disponibles (cada 2 bytes)
        for (size_t i = 2; i + 1 < bytes1.size(); i += 2) {
            try {
                int voltageByte = std::stoi(bytes1[i], nullptr, 16);
                int trimByte = std::stoi(bytes1[i+1], nullptr, 16);
                
                // Si ambos son FF, no hay sensor
                if (voltageByte == 0xFF && trimByte == 0xFF) {
                    continue;
                }
                
                OxygenSensor sensor;
                sensor.bank = 1;
                sensor.sensor = static_cast<int>((i - 2) / 2 + 1);
                sensor.voltage = voltageByte / 200.0;
                sensor.shortTermTrim = (trimByte * 100.0 / 128.0) - 100;
                sensors.push_back(sensor);
            } catch (...) {}
        }
    }
    
    // Procesar banco 2
    std::string r2 = send("0115");
    auto bytes2 = splitResponse(r2);
    
    if (bytes2.size() >= 4 && bytes2[0] == "41" && bytes2[1] == "15") {
        for (size_t i = 2; i + 1 < bytes2.size(); i += 2) {
            try {
                int voltageByte = std::stoi(bytes2[i], nullptr, 16);
                int trimByte = std::stoi(bytes2[i+1], nullptr, 16);
                
                if (voltageByte == 0xFF && trimByte == 0xFF) {
                    continue;
                }
                
                OxygenSensor sensor;
                sensor.bank = 2;
                sensor.sensor = static_cast<int>((i - 2) / 2 + 1);
                sensor.voltage = voltageByte / 200.0;
                sensor.shortTermTrim = (trimByte * 100.0 / 128.0) - 100;
                sensors.push_back(sensor);
            } catch (...) {}
        }
    }
    
    return sensors;
}
*/



// ================= DTCs =================

std::string ELM327::decodeDTCCode(const std::string& code) {
    if (code.length() < 4) return "Código inválido";
    
    static const std::map<std::string, std::string> dtcTypes = {
        {"P0", "Powertrain - Genérico"},
        {"P1", "Powertrain - Fabricante"},
        {"P2", "Powertrain - Genérico (P2xxx)"},
        {"P3", "Powertrain - Genérico (P3xxx)"},
        {"C0", "Chasis - Genérico"},
        {"C1", "Chasis - Fabricante"},
        {"C2", "Chasis - Genérico (C2xxx)"},
        {"C3", "Chasis - Genérico (C3xxx)"},
        {"B0", "Carrocería - Genérico"},
        {"B1", "Carrocería - Fabricante"},
        {"B2", "Carrocería - Genérico (B2xxx)"},
        {"B3", "Carrocería - Genérico (B3xxx)"},
        {"U0", "Red - Genérico"},
        {"U1", "Red - Fabricante"},
        {"U2", "Red - Genérico (U2xxx)"},
        {"U3", "Red - Genérico (U3xxx)"}
    };
    
    std::string prefix = code.substr(0, 2);
    auto it = dtcTypes.find(prefix);
    if (it != dtcTypes.end()) {
        return code + " - " + it->second;
    }
    
    return code;
}

std::vector<DTCCode> ELM327::getDTCs()
{
    std::vector<DTCCode> dtcs;
    
    // Modo 03 - Obtener DTCs almacenados
    std::string r = send("03", 500);
    
    if (r.find("43") != std::string::npos || r.find("NO DATA") != std::string::npos) {
        auto bytes = splitResponse(r);
        
        if (bytes.size() >= 2 && bytes[0] == "43") {
            int numCodes = std::stoi(bytes[1], nullptr, 16);
            int expectedBytes = 2 + (numCodes * 2);
            
            for (size_t i = 2; i + 1 < bytes.size() && i < expectedBytes; i += 2) {
                std::string code = bytes[i] + bytes[i + 1];
                if (code != "0000" && code != "FFFF") {
                    DTCCode dtc;
                    dtc.code = code;
                    dtc.description = decodeDTCCode(code);
                    dtcs.push_back(dtc);
                }
            }
        }
    }
    
    if (dtcs.empty()) {
        DTCCode noDTC;
        noDTC.code = "NONE";
        noDTC.description = "No hay códigos de error almacenados";
        dtcs.push_back(noDTC);
    }
    
    return dtcs;
}

bool ELM327::clearDTCs()
{
    std::cout << "[INFO] Inicializando ELM327..." << std::endl;

    // Reset y configuración básica
    send("ATZ", 2000);    // Reset
    send("ATE0", 500);    // Echo OFF
    send("ATL0", 500);    // Linefeeds OFF
    send("ATS0", 500);    // Spaces OFF
    send("ATH1", 500);    // Headers ON (útil para debug)
    send("ATSP0", 1000);  // Auto protocolo

    // Limpiar buffer
    send("ATD", 500);

    std::cout << "[INFO] Limpiando códigos de error..." << std::endl;

    // Intentar varias veces (algunos ECUs lo requieren)
    for (int i = 0; i < 3; i++) {

        std::string r = send("04", 1500);

        std::cout << "[DEBUG] Respuesta: " << r << std::endl;

        // Respuesta válida según OBD-II:
        // 44 = respuesta positiva a modo 04
        if (r.find("44") != std::string::npos) {
            std::cout << "[OK] Códigos de error borrados correctamente" << std::endl;
            return true;
        }

        // Algunos adaptadores devuelven OK
        if (r.find("OK") != std::string::npos) {
            std::cout << "[OK] Comando aceptado (OK)" << std::endl;
            return true;
        }

        // Si hay "NO DATA" o "ERROR", reintentar
        if (r.find("NO DATA") != std::string::npos || r.find("ERROR") != std::string::npos) {
            std::cout << "[WARN] Reintentando limpieza..." << std::endl;
        }

        // pequeña pausa
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "[ERROR] No se pudieron borrar los códigos" << std::endl;
    return false;
}
 
// ================= DIAGNÓSTICO =================

std::string ELM327::getProtocol()
{
    std::string r = send("ATDP", 100);
    r.erase(std::remove(r.begin(), r.end(), '\r'), r.end());
    r.erase(std::remove(r.begin(), r.end(), '\n'), r.end());
    return r;
}



std::string ELM327::getVehicleInfo()
{
    std::string info = "=== INFORMACIÓN DEL VEHÍCULO ===\n";
    
    // Protocolo
    info += "Protocolo: " + getProtocol() + "\n";
    
    // VIN si está disponible - con manejo de excepciones
    try {
        std::string vin = getVIN();
        if (!vin.empty() && vin != "No disponible" && vin != "ERROR") {
            info += "VIN: " + vin + "\n";
        } else {
            info += "VIN: No disponible\n";
        }
    } catch (const std::exception& e) {
        info += "VIN: Error al leer (" + std::string(e.what()) + ")\n";
    }
    
    // MIL status
    try {
        if (checkMIL()) {
            info += "⚠️  MIL activa - Revisar códigos de error\n";
        } else {
            info += "✓ MIL inactiva\n";
        }
    } catch (const std::exception& e) {
        info += "MIL: Error al consultar\n";
    }
    
    return info;
}

 
 
std::string ELM327::getVIN()
{
    // Primero, obtener la respuesta cruda sin procesar
    std::string fullCmd = "0902\r";
    write(sock, fullCmd.c_str(), fullCmd.size());
    usleep(500000);
    
    // Leer respuesta cruda
    char buffer[1024];
    int n = read(sock, buffer, sizeof(buffer)-1);
    if (n <= 0) return "No disponible";
    
    buffer[n] = '\0';
    std::string response(buffer);
    
    // Limpiar solo caracteres de control básicos
    response.erase(std::remove(response.begin(), response.end(), '\r'), response.end());
    response.erase(std::remove(response.begin(), response.end(), '\n'), response.end());
    
    std::cout << "[DEBUG VIN RAW] " << response << std::endl;
    
    // Buscar el patrón de respuesta correcta
    // Formato: 49 02 01 [bytes ASCII del VIN]
    std::string vin;
    size_t pos = response.find("4902");
    
    if (pos != std::string::npos) {
        // Extraer la parte después de 49 02
        std::string data = response.substr(pos + 4);
        
        // Eliminar espacios para procesar
        data.erase(std::remove(data.begin(), data.end(), ' '), data.end());
        
        // Procesar bytes en pares
        for (size_t i = 0; i < data.length(); i += 2) {
            if (i + 2 <= data.length()) {
                std::string byteStr = data.substr(i, 2);
                // Ignorar el primer byte (que podría ser el contador)
                if (i >= 2) {  // Saltar el byte 0x01 que indica inicio de datos
                    try {
                        int val = std::stoi(byteStr, nullptr, 16);
                        if (val >= 32 && val <= 126) {
                            vin += static_cast<char>(val);
                        }
                    } catch (...) {
                        // Si no se puede convertir, terminar
                        break;
                    }
                }
            }
        }
    }
    
    // Si no se encontró VIN con el método anterior, intentar método alternativo
    if (vin.empty()) {
        // Buscar secuencia de bytes que sean ASCII válidos
        std::string cleanResp;
        for (char c : response) {
            if (isxdigit(c) || c == ' ') {
                cleanResp += c;
            }
        }
        
        std::stringstream ss(cleanResp);
        std::string byte;
        std::vector<std::string> bytes;
        
        while (ss >> byte) {
            if (byte.length() == 2) {
                bytes.push_back(byte);
            }
        }
        
        // Buscar el patrón 49 02 y luego extraer ASCII
        for (size_t i = 0; i < bytes.size() - 1; i++) {
            if (bytes[i] == "49" && bytes[i+1] == "02") {
                for (size_t j = i + 2; j < bytes.size(); j++) {
                    try {
                        int val = std::stoi(bytes[j], nullptr, 16);
                        if (val >= 32 && val <= 126) {
                            vin += static_cast<char>(val);
                        } else if (val == 0 || val == 0xFF) {
                            break;
                        }
                    } catch (...) {
                        break;
                    }
                }
                break;
            }
        }
    }
    
    // Limpiar caracteres no deseados
    if (!vin.empty()) {
        // Eliminar cualquier carácter no imprimible
        vin.erase(std::remove_if(vin.begin(), vin.end(), 
            [](char c) { return !isprint(c); }), vin.end());
        return vin;
    }
    
    return "No disponible";
}
 

bool ELM327::checkMIL()
{
    // PID 0x01 - Estado de diagnóstico
    std::string r = send("0101");
    auto bytes = splitResponse(r);
    
    if (bytes.size() >= 3) {
        try {
            int status = std::stoi(bytes[2], nullptr, 16);
            return (status & 0x80) != 0; // Bit 7 = MIL
        } catch (...) {
            return false;
        }
    }
    
    return false;
}

bool ELM327::setProtocol(int protocol)
{
    std::string cmd = "ATSP" + std::to_string(protocol);
    std::string r = send(cmd, 500);
    return r.find("OK") != std::string::npos;
}

bool ELM327::resetELM()
{
    std::string r = send("ATZ", 2000);
    return r.find("ELM327") != std::string::npos;
}
 

// ================= COMANDOS GM (MODO 22) =================
std::string ELM327::sendCommand(const std::string& pidHex)
{
    if (!isConnected()) return "";

    // Guardar estado actual del cabezal
    std::string oldHeader = send("AT SH", 50);
    std::string oldCRA = send("AT CRA", 50);

    // Configurar cabezal para la ECU del motor (GM)
    send("AT SH 7E0");   // Cabezal de solicitud
    send("AT CRA 7E8");  // Cabezal de respuesta
    usleep(50000);       // Pequeña pausa

    // Construir comando modo 22
    std::string cmd = "22 " + pidHex;
    std::cout << "[TX GM] " << cmd << std::endl;

    // Enviar comando
    std::string full = cmd + "\r";
    ::write(sock, full.c_str(), full.size());

    // Leer respuesta con timeout (500ms)
    char buffer[512];
    std::string response;
    fd_set fds;
    struct timeval tv;
    int ret;

    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 500000;

    while (true) {
        ret = select(sock + 1, &fds, NULL, NULL, &tv);
        if (ret <= 0) break;

        int n = ::recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;

        buffer[n] = '\0';
        response += buffer;

        if (response.find(">") != std::string::npos) break;
    }

    // Restaurar cabezal original
    send(oldHeader, 50);
    send(oldCRA, 50);

    // Limpiar respuesta
    response.erase(std::remove(response.begin(), response.end(), '\r'), response.end());
    response.erase(std::remove(response.begin(), response.end(), '\n'), response.end());
    response.erase(std::remove(response.begin(), response.end(), '>'), response.end());

    // Mostrar diagnóstico
    std::cout << "[RX GM] " << response << std::endl;

    // Detectar error
    if (response.find("7F") == 0) {
        std::cerr << "⚠️  Error en comando GM: PID no soportado o comunicación fallida" << std::endl;
        return "";
    }

    return response;
}



// ================= FUEL TRIM =================

double ELM327::getShortTermTrimBank1() {
    // PID 0114, primer sensor (banco 1, sensor 1)
    std::string r = send("0114");
    auto bytes = splitResponse(r);
    // Respuesta esperada: 41 14 [voltaje1] [trim1] ...
    if (bytes.size() >= 4 && bytes[0] == "41" && bytes[1] == "14") {
        try {
            int trimByte = std::stoi(bytes[3], nullptr, 16);
            return (trimByte * 100.0 / 128.0) - 100;
        } catch (...) { return -1.0; }
    }
    return -1.0;
}

double ELM327::getShortTermTrimBank2() {
    std::string r = send("0115");
    auto bytes = splitResponse(r);
    if (bytes.size() >= 4 && bytes[0] == "41" && bytes[1] == "15") {
        try {
            int trimByte = std::stoi(bytes[3], nullptr, 16);
            return (trimByte * 100.0 / 128.0) - 100;
        } catch (...) { return -1.0; }
    }
    return -1.0;
}

double ELM327::getLongTermTrimBank1() {
    std::string r = send("0107");
    auto bytes = splitResponse(r);
    if (bytes.size() >= 3) {
        try {
            int trim = std::stoi(bytes[2], nullptr, 16);
            return (trim * 100.0 / 128.0) - 100;
        } catch (...) { return -1.0; }
    }
    return -1.0;
}

double ELM327::getLongTermTrimBank2() {
    std::string r = send("0108");
    auto bytes = splitResponse(r);
    if (bytes.size() >= 3) {
        try {
            int trim = std::stoi(bytes[2], nullptr, 16);
            return (trim * 100.0 / 128.0) - 100;
        } catch (...) { return -1.0; }
    }
    return -1.0;
}

FuelTrim ELM327::getAllFuelTrims() {
    FuelTrim ft;
    ft.shortTermBank1 = getShortTermTrimBank1();
    ft.shortTermBank2 = getShortTermTrimBank2();
    ft.longTermBank1 = getLongTermTrimBank1();
    ft.longTermBank2 = getLongTermTrimBank2();
    ft.available = (ft.shortTermBank1 != -1.0 || ft.shortTermBank2 != -1.0 ||
                    ft.longTermBank1 != -1.0 || ft.longTermBank2 != -1.0);
    return ft;
}

OxygenSensor ELM327::getO2Sensor(int bank, int sensor) {
    OxygenSensor empty;
    empty.bank = bank;
    empty.sensor = sensor;
    empty.voltage = -1.0;
    empty.shortTermTrim = -1.0;
    
    std::string pid;
    if (bank == 1) {
        if (sensor <= 4) pid = "0114";
        else if (sensor <= 8) pid = "0116";
        else return empty;
    } else if (bank == 2) {
        if (sensor <= 4) pid = "0115";
        else if (sensor <= 8) pid = "0117";
        else return empty;
    } else return empty;
    
    std::string r = send(pid);
    auto bytes = splitResponse(r);
    // Formato: 41 14 v1 t1 v2 t2 v3 t3 v4 t4
    int index = 2 + (sensor-1)*2;
    if (bytes.size() > index+1 && bytes[0] == "41" && (bytes[1] == pid.substr(2,2) || bytes[1] == pid.substr(2))) {
        try {
            int vByte = std::stoi(bytes[index], nullptr, 16);
            int tByte = std::stoi(bytes[index+1], nullptr, 16);
            empty.voltage = vByte / 200.0;
            empty.shortTermTrim = (tByte * 100.0 / 128.0) - 100;
        } catch (...) {}
    }
    return empty;
}

// ================= LOGGING COMPLETO CON HEXADECIMAL =================

void ELM327::logAllSensorsRaw(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ERROR] No se pudo crear " << filename << std::endl;
        return;
    }
    
    // Cabecera: timestamp, PID, comando, respuesta_hex, valor_interpretado
    file << "timestamp,pid,command,hex_response,interpreted_value\n";
    
    // Lista de PIDs a consultar (nombre, comando, función de interpretación)
    struct SensorInfo {
        std::string name;
        std::string cmd;
        std::function<std::string()> interpreter; // lambda que devuelve string
    };
    
    // Usamos capturas de this para llamar a los métodos
    std::vector<SensorInfo> sensors = {
        {"RPM", "010C", [this]() { return std::to_string(getRPM()); }},
        {"Speed", "010D", [this]() { return std::to_string(getSpeed()); }},
        {"CoolantTemp", "0105", [this]() { return std::to_string(getCoolantTemp()); }},
        {"EngineLoad", "0104", [this]() { return std::to_string(getEngineLoad()); }},
        {"Throttle", "0111", [this]() { return std::to_string(getThrottlePosition()); }},
        {"IntakePressure", "010B", [this]() { return std::to_string(getIntakePressure()); }},
        {"IntakeTemp", "010F", [this]() { return std::to_string(getIntakeTemp()); }},
        {"TimingAdvance", "010E", [this]() { return std::to_string(getTimingAdvance()); }},
        {"MAF", "0110", [this]() { return std::to_string(getMAF()); }},
        {"FuelLevel", "012F", [this]() { return std::to_string(getFuelLevel()); }},
        {"BaroPressure", "0133", [this]() { return std::to_string(getBarometricPressure()); }},
        {"LTFT_Bank1", "0107", [this]() { return std::to_string(getLongTermTrimBank1()); }},
        {"LTFT_Bank2", "0108", [this]() { return std::to_string(getLongTermTrimBank2()); }},
        {"STFT_Bank1_S1", "0114", [this]() { return std::to_string(getShortTermTrimBank1()); }},
        {"STFT_Bank2_S1", "0115", [this]() { return std::to_string(getShortTermTrimBank2()); }},
        {"O2_B1S1_Voltage", "0114", [this]() { return std::to_string(getO2Sensor(1,1).voltage); }},
        {"O2_B2S1_Voltage", "0115", [this]() { return std::to_string(getO2Sensor(2,1).voltage); }}
    };
    
    time_t now = time(nullptr);
    for (auto& s : sensors) {
        // Enviar comando y obtener respuesta cruda
        std::string fullCmd = s.cmd + "\r";
        write(sock, fullCmd.c_str(), fullCmd.size());
        usleep(200000);
        std::string rawResp = readRaw();
        
        // Valor interpretado
        std::string value = s.interpreter();
        
        file << now << ",\"" << s.name << "\",\"" << s.cmd << "\",\""
             << rawResp << "\",\"" << value << "\"\n";
        file.flush();
    }
    
    file.close();
    std::cout << "[OK] Log completo guardado en " << filename << std::endl;
}

// ================= LOG ESPECÍFICO PARA P0171 (POBRE) =================

void ELM327::logP0171Diagnostic(const std::string& filename, int durationSec) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ERROR] No se pudo crear " << filename << std::endl;
        return;
    }
    
    // Cabecera
    file << "timestamp,rpm,speed,stft_b1(%),stft_b2(%),ltft_b1(%),ltft_b2(%),o2_b1s1_voltage(V),o2_b2s1_voltage(V)\n";
    
    time_t start = time(nullptr);
    int count = 0;
    
    std::cout << "[INFO] Registrando datos para diagnóstico P0171 durante " << durationSec << " segundos...\n";
    std::cout << "Presione Ctrl+C para detener antes.\n";
    
    while (time(nullptr) - start < durationSec) {
        time_t now = time(nullptr);
        
        int rpm = getRPM();
        int speed = getSpeed();
        double stft1 = getShortTermTrimBank1();
        double stft2 = getShortTermTrimBank2();
        double ltft1 = getLongTermTrimBank1();
        double ltft2 = getLongTermTrimBank2();
        double o2v1 = getO2Sensor(1,1).voltage;
        double o2v2 = getO2Sensor(2,1).voltage;
        
        file << now << ","
             << rpm << ","
             << speed << ","
             << stft1 << ","
             << stft2 << ","
             << ltft1 << ","
             << ltft2 << ","
             << o2v1 << ","
             << o2v2 << "\n";
        file.flush();
        
        count++;
        if (count % 10 == 0) {
            std::cout << "Registros: " << count << "\r" << std::flush;
        }
        
        sleep(1); // cada segundo
    }
    
    file.close();
    std::cout << "\n[OK] Log P0171 guardado en " << filename << " (" << count << " registros)\n";
}