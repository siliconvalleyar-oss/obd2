//gm_commands.cpp

#include "gm_commands.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <unistd.h>      // Para usleep
#include <sys/select.h>  // Para select
#include <sys/time.h>    // Para timeval

GMCommands::GMCommands(ELM327* elm327) : elm(elm327) {
    oldHeader = "";
    oldCRA = "";
}

GMCommands::~GMCommands() {
    restoreHeader();
}

bool GMCommands::setupGMHeader() {
    // Guardar configuración actual
    oldHeader = elm->send("AT SH", 50);
    oldCRA = elm->send("AT CRA", 50);
    
    // Configurar para ECU del motor GM
    elm->send("AT SH 7E0");   // Cabezal de solicitud
    elm->send("AT CRA 7E8");  // Cabezal de respuesta
    usleep(50000);
    
    return true;
}

bool GMCommands::restoreHeader() {
    if (!oldHeader.empty()) {
        elm->send(oldHeader, 50);
    }
    if (!oldCRA.empty()) {
        elm->send(oldCRA, 50);
    }
    return true;
}

std::string GMCommands::processResponse(const std::string& response, const std::string& expectedPrefix) {
    if (response.empty()) {
        return "";
    }
    
    // Verificar error (7F)
    if (response.find("7F") == 0) {
        return "";
    }
    
    // Verificar prefijo esperado
    if (!expectedPrefix.empty() && response.find(expectedPrefix) != 0) {
        return "";
    }
    
    return response;
}

void GMCommands::showError(const std::string& cmd, const std::string& response) {
    std::cerr << "\n⚠️  Error en comando GM: " << cmd << std::endl;
    if (response.find("7F") == 0) {
        std::cerr << "   PID no soportado por la ECU" << std::endl;
    } else if (response.empty()) {
        std::cerr << "   Sin respuesta (timeout)" << std::endl;
    } else {
        std::cerr << "   Respuesta: " << response << std::endl;
    }
}


std::string GMCommands::sendCommand(const std::string& pidHex, bool useGMHeader) {
    if (!elm->isOnline()) {
        return "Error: No hay conexión";
    }
    if (useGMHeader) setupGMHeader();
    std::string response = elm->send("22 " + pidHex, 500);
    if (useGMHeader) restoreHeader();
    
    if (response.empty()) return "Sin respuesta (timeout)";
    if (response.find("7F") == 0) {
        // Decodificar error OBD
        if (response.find("7F2231") != std::string::npos)
            return "Modo 22 no soportado por esta ECU";
        else
            return "Error: " + response;
    }
    return response;
}



// Decodificar kilometraje (F190)
std::string GMCommands::decodeKilometers(const std::string& hexResponse) {
    // Formato esperado: 62 F190 [bytes de datos]
    if (hexResponse.length() < 10) return "Datos insuficientes";
    
    try {
        // Buscar después de "62F190"
        size_t pos = hexResponse.find("62F190");
        if (pos == std::string::npos) return "Formato inválido";
        
        std::string data = hexResponse.substr(pos + 6);
        
        // Los datos pueden ser 4 bytes (32 bits) para kilometraje
        if (data.length() >= 8) {
            uint32_t km = 0;
            for (size_t i = 0; i < 4 && i * 2 < data.length(); i++) {
                std::string byteStr = data.substr(i * 2, 2);
                km = (km << 8) | std::stoi(byteStr, nullptr, 16);
            }
            
            // El valor puede estar en km o en unidades de 100m
            std::stringstream ss;
            if (km > 1000000) {
                ss << (km / 1000.0) << " km (aprox)";
            } else {
                ss << km << " km";
            }
            return ss.str();
        }
    } catch (const std::exception& e) {
        return "Error decodificando: " + std::string(e.what());
    }
    
    return hexResponse;
}

// Decodificar temperatura catalizador (F40C)
std::string GMCommands::decodeCatalystTemp(const std::string& hexResponse) {
    if (hexResponse.length() < 10) return "Datos insuficientes";
    
    try {
        size_t pos = hexResponse.find("62F40C");
        if (pos == std::string::npos) return "Formato inválido";
        
        std::string data = hexResponse.substr(pos + 6);
        
        if (data.length() >= 4) {
            int temp = std::stoi(data.substr(0, 4), nullptr, 16);
            // Temperatura en grados Celsius (puede ser en décimas)
            if (temp > 1000) {
                temp = temp / 10;
            }
            std::stringstream ss;
            ss << temp << " °C";
            return ss.str();
        }
    } catch (const std::exception& e) {
        return "Error decodificando";
    }
    
    return hexResponse;
}

// Decodificar presión combustible (F423)
std::string GMCommands::decodeFuelPressure(const std::string& hexResponse) {
    if (hexResponse.length() < 10) return "Datos insuficientes";
    
    try {
        size_t pos = hexResponse.find("62F423");
        if (pos == std::string::npos) return "Formato inválido";
        
        std::string data = hexResponse.substr(pos + 6);
        
        if (data.length() >= 4) {
            int pressure = std::stoi(data.substr(0, 4), nullptr, 16);
            // Presión en kPa (puede ser en décimas)
            std::stringstream ss;
            if (pressure > 1000) {
                ss << (pressure / 10.0) << " kPa";
            } else {
                ss << pressure << " kPa";
            }
            return ss.str();
        }
    } catch (const std::exception& e) {
        return "Error decodificando";
    }
    
    return hexResponse;
}

// Decodificar torque motor (F40D)
std::string GMCommands::decodeEngineTorque(const std::string& hexResponse) {
    if (hexResponse.length() < 10) return "Datos insuficientes";
    
    try {
        size_t pos = hexResponse.find("62F40D");
        if (pos == std::string::npos) return "Formato inválido";
        
        std::string data = hexResponse.substr(pos + 6);
        
        if (data.length() >= 4) {
            int torque = std::stoi(data.substr(0, 4), nullptr, 16);
            std::stringstream ss;
            ss << torque << " Nm";
            return ss.str();
        }
    } catch (const std::exception& e) {
        return "Error decodificando";
    }
    
    return hexResponse;
}

// Decodificar voltaje ECU (F40A)
std::string GMCommands::decodeECUVoltage(const std::string& hexResponse) {
    if (hexResponse.length() < 10) return "Datos insuficientes";
    
    try {
        size_t pos = hexResponse.find("62F40A");
        if (pos == std::string::npos) return "Formato inválido";
        
        std::string data = hexResponse.substr(pos + 6);
        
        if (data.length() >= 4) {
            int voltage = std::stoi(data.substr(0, 4), nullptr, 16);
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << (voltage / 100.0) << " V";
            return ss.str();
        }
    } catch (const std::exception& e) {
        return "Error decodificando";
    }
    
    return hexResponse;
}

// Decodificar historial GM (19 02)
std::string GMCommands::decodeGMHistory(const std::string& hexResponse) {
    if (hexResponse.empty()) return "Sin datos";
    
    try {
        size_t pos = hexResponse.find("5902");
        if (pos == std::string::npos) return "Formato inválido";
        
        std::string data = hexResponse.substr(pos + 4);
        
        if (data.empty()) return "Sin códigos almacenados";
        
        // Formatear códigos de error
        std::string result;
        for (size_t i = 0; i + 4 <= data.length(); i += 4) {
            if (!result.empty()) result += ", ";
            result += data.substr(i, 4);
        }
        
        return result.empty() ? "Sin códigos" : result;
    } catch (const std::exception& e) {
        return "Error decodificando";
    }
}

std::string GMCommands::getKilometers() {
    std::string response = sendCommand("F190");
    if (response.find("no soportado") != std::string::npos || response.find("Error") != std::string::npos)
        return response;
    return decodeKilometers(response);
}

std::string GMCommands::getCatalystTemp() {
    std::string response = sendCommand("F40C");
    return decodeCatalystTemp(response);
}

std::string GMCommands::getFuelPressure() {
    std::string response = sendCommand("F423");
    return decodeFuelPressure(response);
}

std::string GMCommands::getEngineTorque() {
    std::string response = sendCommand("F40D");
    return decodeEngineTorque(response);
}

std::string GMCommands::getECUVoltage() {
    std::string response = sendCommand("F40A");
    return decodeECUVoltage(response);
}

std::string GMCommands::getGMHistory() {
    std::string response = sendCommand("19 02", true);
    return decodeGMHistory(response);
}

std::string GMCommands::clearGMHistory() {
    std::string response = sendCommand("14 FF FF", true);
    if (!response.empty() && response.find("7F") != 0) {
        return "Historial borrado correctamente";
    }
    return "Error al borrar historial";
}

// --- Reset adaptativos Chevrolet/GM ---

// Reset adaptativos método 1 - Comando 22 F10E (Chevrolet/GM)
std::string GMCommands::resetAdaptations() {
    std::cout << "\n🔧 Ejecutando reset adaptativos (22 F10E)..." << std::endl;
    std::string response = sendCommand("F10E", true);
    if (!response.empty() && response.find("7F") != 0) {
        return "✅ Reset adaptativos (F10E) completado: " + response;
    }
    return "❌ Error al resetear adaptaciones (F10E): " + response;
}

// Reset adaptativos método 2 - Comando 22 F10D
std::string GMCommands::resetAdaptations2() {
    std::cout << "\n🔧 Ejecutando reset adaptativos (22 F10D)..." << std::endl;
    std::string response = sendCommand("F10D", true);
    if (!response.empty() && response.find("7F") != 0) {
        return "✅ Reset adaptativos (F10D) completado: " + response;
    }
    return "❌ Error al resetear adaptaciones (F10D): " + response;
}

// Reset Fuel Trims - Prueba F11C, luego F10B, luego F117
std::string GMCommands::resetFuelTrims() {
    std::string cmds[] = {"F11C", "F10B", "F117"};
    std::string labels[] = {"F11C", "F10B", "F117"};
    
    for (int i = 0; i < 3; i++) {
        std::cout << "\n🔧 Intentando reset Fuel Trims (" << labels[i] << ")..." << std::endl;
        std::string response = sendCommand(cmds[i], true);
        if (!response.empty() && response.find("7F") != 0) {
            return "✅ Reset Fuel Trims (" + labels[i] + ") completado: " + response;
        }
        std::cout << "   ⚠️  Comando " << labels[i] << " no soportado, probando siguiente..." << std::endl;
    }
    return "❌ Error: Ningún comando de reset Fuel Trims fue soportado por la ECU";
}

// Reset completo - Ejecuta todos los métodos y verifica con LTFT
std::string GMCommands::resetAllAdaptations() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "   RESET COMPLETO DE ADAPTACIONES" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::string results;
    bool anySuccess = false;
    
    // 1. Reset método 1 (F10E)
    std::cout << "\n📌 Paso 1/4: Reset adaptativos (F10E)" << std::endl;
    std::string r1 = resetAdaptations();
    std::cout << r1 << std::endl;
    if (r1.find("✅") != std::string::npos) anySuccess = true;
    results += "• " + r1 + "\n";
    
    // 2. Reset método 2 (F10D)
    std::cout << "\n📌 Paso 2/4: Reset adaptativos (F10D)" << std::endl;
    std::string r2 = resetAdaptations2();
    std::cout << r2 << std::endl;
    if (r2.find("✅") != std::string::npos) anySuccess = true;
    results += "• " + r2 + "\n";
    
    // 3. Reset Fuel Trims
    std::cout << "\n📌 Paso 3/4: Reset Fuel Trims" << std::endl;
    std::string r3 = resetFuelTrims();
    std::cout << r3 << std::endl;
    if (r3.find("✅") != std::string::npos) anySuccess = true;
    results += "• " + r3 + "\n";
    
    // 4. Verificación con LTFT (Long Term Fuel Trim) - modo estándar OBD
    std::cout << "\n📌 Paso 4/4: Verificación LTFT" << std::endl;
    std::cout << "   Leyendo Fuel Trim a largo plazo (modo 01 PID 07)..." << std::endl;
    elm->send("AT SH 7E0");   // Cabezal de solicitud GM
    elm->send("AT CRA 7E8");  // Cabezal de respuesta
    usleep(50000);
    std::string ltftResponse = elm->send("01 07", 500);  // LTFT banco 1 (modo OBD estándar)
    std::cout << "   LTFT response: " << ltftResponse << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "   RESUMEN DEL RESET COMPLETO" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << results << std::endl;
    
    if (anySuccess) {
        return "✅ Reset completo finalizado con éxito";
    }
    return "❌ Reset completo: ningún método fue soportado por esta ECU";
}

// --- Nuevas lecturas GM ---

// Decodificar temperatura transmisión (22 1940)
// Formula: ((A*256)+B)/10 - 40 = °C
std::string GMCommands::decodeTransmissionTemp(const std::string& hexResponse) {
    if (hexResponse.length() < 10) return "Datos insuficientes";
    
    try {
        size_t pos = hexResponse.find("621940");
        if (pos == std::string::npos) return "Formato inválido";
        
        std::string data = hexResponse.substr(pos + 6);
        
        if (data.length() >= 4) {
            int a = std::stoi(data.substr(0, 2), nullptr, 16);
            int b = std::stoi(data.substr(2, 2), nullptr, 16);
            double temp = ((a * 256.0) + b) / 10.0 - 40.0;
            
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << temp << " °C";
            return ss.str();
        }
    } catch (const std::exception& e) {
        return "Error decodificando";
    }
    
    return hexResponse;
}

// Decodificar presión aceite (22 1470)
// Formula: A = kPa (o (A*256)+B para 2 bytes)
std::string GMCommands::decodeOilPressure(const std::string& hexResponse) {
    if (hexResponse.length() < 10) return "Datos insuficientes";
    
    try {
        size_t pos = hexResponse.find("621470");
        if (pos == std::string::npos) return "Formato inválido";
        
        std::string data = hexResponse.substr(pos + 6);
        
        if (data.length() >= 4) {
            int a = std::stoi(data.substr(0, 2), nullptr, 16);
            int b = std::stoi(data.substr(2, 2), nullptr, 16);
            // Intentar 1 byte primero, si es 0 probar 2 bytes
            double pressure = a;
            if (a == 0 && b > 0) {
                pressure = (a * 256.0 + b);
            }
            double psi = pressure * 0.145038; // kPa a psi
            
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << pressure << " kPa (" << psi << " psi)";
            return ss.str();
        }
    } catch (const std::exception& e) {
        return "Error decodificando";
    }
    
    return hexResponse;
}

// Decodificar Knock Retard (22 11A2)
// Formula: A*0.35 = grados
std::string GMCommands::decodeKnockRetard(const std::string& hexResponse) {
    if (hexResponse.length() < 10) return "Datos insuficientes";
    
    try {
        size_t pos = hexResponse.find("6211A2");
        if (pos == std::string::npos) return "Formato inválido";
        
        std::string data = hexResponse.substr(pos + 6);
        
        if (data.length() >= 2) {
            int a = std::stoi(data.substr(0, 2), nullptr, 16);
            double retard = a * 0.35;
            
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << retard << "°";
            return ss.str();
        }
    } catch (const std::exception& e) {
        return "Error decodificando";
    }
    
    return hexResponse;
}

std::string GMCommands::getTransmissionTemp() {
    std::string response = sendCommand("1940");
    if (response.find("no soportado") != std::string::npos || response.find("Error") != std::string::npos)
        return response;
    std::string decoded = decodeTransmissionTemp(response);
    if (decoded == "Datos insuficientes" || decoded == "Formato inválido" ||
        decoded == "Error decodificando") {
        return response;
    }
    return decoded;
}

std::string GMCommands::getOilPressure() {
    std::string response = sendCommand("1470");
    if (response.find("no soportado") != std::string::npos || response.find("Error") != std::string::npos)
        return response;
    std::string decoded = decodeOilPressure(response);
    if (decoded == "Datos insuficientes" || decoded == "Formato inválido" ||
        decoded == "Error decodificando") {
        return response;
    }
    return decoded;
}

std::string GMCommands::getKnockRetard() {
    std::string response = sendCommand("11A2");
    if (response.find("no soportado") != std::string::npos || response.find("Error") != std::string::npos)
        return response;
    std::string decoded = decodeKnockRetard(response);
    if (decoded == "Datos insuficientes" || decoded == "Formato inválido" ||
        decoded == "Error decodificando") {
        return response;
    }
    return decoded;
}

void GMCommands::scanPIDs(uint16_t start, uint16_t end) {
    std::cout << "\n=== ESCANEO DE PIDs GM ===\n";
    std::cout << "Rango: 0x" << std::hex << start << " - 0x" << end << std::dec << "\n\n";
    
    int found = 0;
    
    for (uint16_t pid = start; pid <= end; pid++) {
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << pid;
        
        std::string response = sendCommand(ss.str());
        
        if (!response.empty() && response.find("7F") != 0 && response.find("62") == 0) {
            found++;
            std::cout << "✓ PID 0x" << ss.str() << " -> " << response << std::endl;
        }
        
        // Pequeña pausa para no saturar
        usleep(50000);
    }
    
    std::cout << "\n=== ESCANEO COMPLETADO ===\n";
    std::cout << "PIDs encontrados: " << found << std::endl;
}
