#pragma once

#include "elm327.hpp"
#include "gm_commands.hpp"
#include "logger.hpp"

#include <iostream>
#include <iomanip>
#include <limits>
#include <memory>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>
#include <unistd.h>
#include <ctime>

// Variable global para controlar el logger (manejada por señal)
static std::atomic<bool> g_runningLogger(true);

// Manejador de señal SIGINT (Ctrl+C)
static void signalHandler(int /*sig*/) {
    g_runningLogger = false;
}

namespace OBD {

/**
 * @brief Clase principal de la aplicación.
 * 
 * Gestiona la conexión con el ELM327, muestra el menú y ejecuta las funciones
 * de diagnóstico según la opción seleccionada.
 */
class App {
public:
    /**
     * @brief Constructor. Inicializa los punteros únicos.
     */
    App() = default;

    /**
     * @brief Punto de entrada principal.
     * 
     * Establece la conexión Bluetooth, configura el logger y ejecuta el bucle principal.
     */
    void run() {
        clearScreen();
        printHeader();

        // Crear y abrir el logger
        m_logger = std::make_unique<Logger>();
        if (!m_logger->open()) {
            std::cerr << "No se pudo crear archivo de log\n";
        }

        // Dirección MAC del ELM327 (se puede pedir al usuario)
        std::string mac = "00:1D:A5:07:23:6E";
        std::cout << "Dirección MAC del ELM327 (ej: 00:1D:A5:07:23:6E): ";
        std::cout << mac << " (usando por defecto)\n";

        // Conectar
        m_elm = std::make_unique<ELM327>(mac);
        if (!m_elm->connectBT()) {
            std::cerr << "\n❌ No se pudo conectar al ELM327\n";
            std::cerr << "Verifique:\n";
            std::cerr << "  - El dispositivo Bluetooth está encendido\n";
            std::cerr << "  - La dirección MAC es correcta\n";
            std::cerr << "  - El ELM327 está emparejado con el sistema\n";
            return;
        }

        std::cout << "\n✅ Conexión establecida correctamente\n";
        std::cout << "📊 Protocolo detectado: " << m_elm->getProtocol() << "\n\n";

        // Inicializar comandos GM
        m_gm = std::make_unique<GMCommands>(m_elm.get());

        // Registrar manejador de señal para el logger
        std::signal(SIGINT, ::signalHandler);

        // Bucle principal del menú
        bool running = true;
        int option;
        while (running) {
            printMenu();
            std::cin >> option;

            if (std::cin.fail()) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Opción inválida\n";
                continue;
            }

            running = processOption(option);

            // Pausa después de la mayoría de opciones (excepto las que son continuas)
            if (running && option != 0 && !isContinuousOption(option)) {
                std::cout << "\nPresione Enter para continuar...";
                std::cin.ignore();
                std::cin.get();
                clearScreen();
                printHeader();
                std::cout << "Protocolo: " << m_elm->getProtocol() << "\n\n";
            }
        }

        std::cout << "✅ Programa finalizado\n";
    }

private:
    std::unique_ptr<ELM327> m_elm;          ///< Controlador ELM327
    std::unique_ptr<GMCommands> m_gm;       ///< Comandos específicos GM
    std::unique_ptr<Logger> m_logger;       ///< Logger CSV

    // ---------------------------------------------------------------
    // Funciones auxiliares de pantalla y menú
    // ---------------------------------------------------------------

    void clearScreen() const {
        std::cout << "\033[2J\033[1;1H";
    }

    void printHeader() const {
        std::cout << "========================================\n";
        std::cout << "   ELM327 OBD-II DIAGNÓSTICO VEHICULAR\n";
        std::cout << "========================================\n\n";
    }

    void printMenu() const {
        std::cout << "\n============== DIAGNOSTICO OBD2 ==============\n";
        std::cout << "1.  RPM motor\n";
        std::cout << "2.  Velocidad vehículo\n";
        std::cout << "3.  Temperatura motor\n";
        std::cout << "4.  Carga motor\n";
        std::cout << "5.  Posición acelerador\n";
        std::cout << "6.  Presión colector\n";
        std::cout << "7.  Temperatura admisión\n";
        std::cout << "8.  Avance encendido\n";
        std::cout << "9.  Flujo aire MAF\n";
        std::cout << "10. Nivel combustible\n";
        std::cout << "11. Sensores oxígeno\n";

        std::cout << "\n============== DIAGNOSTICO DTC ==============\n";
        std::cout << "12. Leer códigos error\n";
        std::cout << "13. Borrar códigos error\n";
        std::cout << "14. Información vehículo\n";

        std::cout << "\n============== ESCANEO COMPLETO ==============\n";
        std::cout << "15. Todos los sensores\n";

        std::cout << "\n============== EXTENSION GM ==================\n";
        std::cout << "16. Kilometraje ECU (GM)\n";
        std::cout << "17. Temperatura catalizador\n";
        std::cout << "18. Presión combustible real\n";
        std::cout << "19. Torque motor ECU\n";
        std::cout << "20. Voltaje ECU\n";
        std::cout << "21. Historial errores GM\n";
        std::cout << "22. Borrar historial GM\n";
        std::cout << "23. Reset adaptativos ECU\n";
        std::cout << "24. Temperatura transmisión (GM)\n";
        std::cout << "25. Presión aceite motor (GM)\n";
        std::cout << "26. Knock retard (GM)\n";
        std::cout << "27. Escaneo automático de PIDs GM\n";

        std::cout << "\n============== MODULOS VEHICULO ==============\n";
        std::cout << "28. Leer modulo ABS\n";
        std::cout << "29. Leer modulo Airbag\n";
        std::cout << "30. Leer modulo BCM\n";

        std::cout << "\n============== HERRAMIENTAS AVANZADAS ==============\n";
        std::cout << "31. Dashboard tiempo real\n";
        std::cout << "32. Osciloscopio O2\n";
        std::cout << "33. Logger motor (CSV)\n";
        std::cout << "34. Detectar módulos CAN\n";
        std::cout << "35. Monitor sensores\n";
        std::cout << "36. Enviar comando personalizado\n";
        std::cout << "37. sensor oxigeno banco 1 y banco2\n";

        std::cout << "38. Log completo (hex + valores) todos sensores\n";
        std::cout << "39. Diagnóstico P0171 (log STFT/LTFT/O2 durante 60s)\n";
        std::cout << "\n0. Salir\n";
        std::cout << "==============================================\n";
        std::cout << "Opción: ";
    }

    /**
     * @brief Determina si una opción debe ejecutarse sin pausa posterior.
     * @param option Número de opción.
     * @return true si es una opción continua (dashboard, osciloscopio, etc.).
     */
    bool isContinuousOption(int option) const {
        return (option == 31 || option == 32 || option == 33 || option == 35);
    }

    // ---------------------------------------------------------------
    // Procesamiento de cada opción del menú
    // ---------------------------------------------------------------

    bool processOption(int option) {
        switch (option) {
            case 1:  readAndLog("RPM", m_elm->getRPM()); break;
            case 2:  readAndLog("Speed", m_elm->getSpeed()); break;
            case 3:  readAndLog("CoolantTemp", m_elm->getCoolantTemp()); break;
            case 4:  readAndLog("EngineLoad", m_elm->getEngineLoad()); break;
            case 5:  readAndLog("ThrottlePosition", m_elm->getThrottlePosition()); break;
            case 6:  readAndLog("IntakePressure", m_elm->getIntakePressure()); break;
            case 7:  readAndLog("IntakeTemp", m_elm->getIntakeTemp()); break;
            case 8:  readAndLog("TimingAdvance", m_elm->getTimingAdvance()); break;
            case 9:  std::cout << "MAF: " << m_elm->getMAF() << " g/s\n"; break;
            case 10: std::cout << "Combustible: " << m_elm->getFuelLevel() << "%\n"; break;
            case 11: showOxygenSensors(); break;
            case 12: showDTCs(); break;
            case 13: clearDTCs(); break;
            case 14: showVehicleInfo(); break;
            case 15: showAllSensors(); break;
            case 16: std::cout << "\nKilometraje ECU: " << m_gm->getKilometers() << std::endl; break;
            case 17: std::cout << "\nTemp. catalizador: " << m_gm->getCatalystTemp() << std::endl; break;
            case 18: std::cout << "\nPresión combustible: " << m_gm->getFuelPressure() << std::endl; break;
            case 19: std::cout << "\nTorque motor: " << m_gm->getEngineTorque() << std::endl; break;
            case 20: std::cout << "\nVoltaje ECU: " << m_gm->getECUVoltage() << std::endl; break;
            case 21: std::cout << "\nHistorial GM: " << m_gm->getGMHistory() << std::endl; break;
            case 22: std::cout << "\n" << m_gm->clearGMHistory() << std::endl; break;
            case 23: showResetAdaptationsMenu(); break;
            case 24: std::cout << "\nTemp. transmisión: " << m_gm->getTransmissionTemp() << std::endl; break;
            case 25: std::cout << "\nPresión aceite: " << m_gm->getOilPressure() << std::endl; break;
            case 26: std::cout << "\nKnock retard: " << m_gm->getKnockRetard() << std::endl; break;
            case 27: m_gm->scanPIDs(); break;
            case 28: readModule("ABS", "7E2"); break;
            case 29: readModule("AIRBAG", "7E4"); break;
            case 30: readModule("BCM", "7E6"); break;
            case 31: liveDashboard(); break;
            case 32: oxygenSensorScope(); break;
            case 33: engineLogger(); break;
            case 34: detectModules(); break;
            case 35: liveSensors(); break;
            case 36: sendCustomCommand(); break;
            case 37: showOxygenSensorsLoop(); break;
            case 38: fullSensorsLog(); break;
            case 39: p0171DiagnosticLog(); break;
            case 0: return false;
            default: std::cout << "Opción inválida\n"; break;
        }
        return true;
    }

    // ---------------------------------------------------------------
    // Métodos de lectura y log
    // ---------------------------------------------------------------

    template<typename T>
    void readAndLog(const std::string& key, T value) {
        std::cout << key << ": " << value << std::endl;
        if (m_logger) m_logger->log(key, value);
    }

    // ---------------------------------------------------------------
    // Funciones específicas de diagnóstico
    // ---------------------------------------------------------------

    void showAllSensors() const {
        std::cout << "\n=== LECTURA COMPLETA DE SENSORES ===\n";
        std::cout << "RPM: " << m_elm->getRPM() << " RPM\n";
        std::cout << "Velocidad: " << m_elm->getSpeed() << " km/h\n";
        std::cout << "Temp. motor: " << m_elm->getCoolantTemp() << " °C\n";
        std::cout << "Carga motor: " << m_elm->getEngineLoad() << "%\n";
        std::cout << "Acelerador: " << std::fixed << std::setprecision(1)
                  << m_elm->getThrottlePosition() << "%\n";
        std::cout << "Presión colector: " << m_elm->getIntakePressure() << " kPa\n";
        std::cout << "Temp. admisión: " << m_elm->getIntakeTemp() << " °C\n";
        std::cout << "Avance encendido: " << m_elm->getTimingAdvance() << "°\n";
        std::cout << "MAF: " << m_elm->getMAF() << " g/s\n";
        std::cout << "Combustible: " << m_elm->getFuelLevel() << "%\n";
        std::cout << "Presión barométrica: " << m_elm->getBarometricPressure() << " kPa\n";
    }

    void showOxygenSensors() const {
        auto sensors = m_elm->getOxygenSensors();
        if (sensors.empty()) {
            std::cout << "\n=== SENSORES DE OXÍGENO ===\n";
            std::cout << "No se detectaron sensores de oxígeno activos\n";
            std::cout << "Posibles causas:\n";
            std::cout << "  - Motor apagado\n";
            std::cout << "  - Sensores no soportados por el vehículo\n";
            std::cout << "  - Sensores en fase de calentamiento\n";
            return;
        }

        std::cout << "\n=== SENSORES DE OXÍGENO ===\n";
        for (const auto& sensor : sensors) {
            std::cout << "Banco " << sensor.bank << " - Sensor " << sensor.sensor << "\n";
            std::cout << "  Voltaje: " << std::fixed << std::setprecision(3)
                      << sensor.voltage << " V";
            if (sensor.voltage < 0.1)      std::cout << " (mezcla pobre)";
            else if (sensor.voltage > 0.9) std::cout << " (mezcla rica)";
            else if (sensor.voltage > 0.1 && sensor.voltage < 0.9) std::cout << " (normal)";
            std::cout << "\n";
            std::cout << "  Trim ST: " << std::fixed << std::setprecision(1)
                      << sensor.shortTermTrim << "%";
            if (sensor.shortTermTrim > 10)       std::cout << " (enriquecimiento)";
            else if (sensor.shortTermTrim < -10) std::cout << " (empobrecimiento)";
            std::cout << "\n\n";
        }
    }

    void showOxygenSensorsLoop() {
        int iterations = 16;
        while (iterations--) {
            clearScreen();
            showOxygenSensors();
            usleep(800000);
            std::cout << std::endl;
        }
    }

    void showDTCs() const {
        auto dtcs = m_elm->getDTCs();
        std::cout << "\n=== CÓDIGOS DE ERROR (DTC) ===\n";
        if (dtcs.size() == 1 && dtcs[0].code == "NONE") {
            std::cout << dtcs[0].description << std::endl;
        } else {
            std::cout << "Total de códigos encontrados: " << dtcs.size() << "\n\n";
            for (const auto& dtc : dtcs) {
                std::cout << "🔧 " << dtc.description << std::endl;
            }
        }
    }

    void clearDTCs() {
        char confirm;
        std::cout << "⚠️  ¿Está seguro de borrar todos los códigos de error? (s/N): ";
        std::cin >> confirm;
        if (confirm == 's' || confirm == 'S') {
            if (m_elm->clearDTCs()) {
                std::cout << "✅ Códigos borrados correctamente\n";
            }
        }
    }

    void showVehicleInfo() const {
        std::string info = m_elm->getVehicleInfo();
        std::cout << info << std::endl;
        if (m_logger) m_logger->log("VehicleInfo", info);
    }

    void readModule(const std::string& name, const std::string& header) {
        m_elm->send("AT SH " + header);
        std::string r = m_elm->send("03");
        std::cout << "\n" << name << ": " << r << std::endl;
    }

    void liveDashboard() const {
        std::cout << "\n=== DASHBOARD MOTOR TIEMPO REAL ===\n";
        std::cout << "Presione Ctrl+C para salir\n\n";
        while (true) {
            std::cout << "\rRPM: " << m_elm->getRPM()
                      << " | SPEED: " << m_elm->getSpeed()
                      << " | TEMP: " << m_elm->getCoolantTemp() << "°C"
                      << " | LOAD: " << m_elm->getEngineLoad() << "%"
                      << std::flush;
            usleep(500000);
        }
    }

    void oxygenSensorScope() const {
        std::cout << "\n=== OSCILOSCOPIO SENSOR O2 ===\n";
        std::cout << "Presione Ctrl+C para salir\n\n";
        while (true) {
            std::string response = m_elm->send("01 14");
            std::cout << response << std::endl;
            usleep(200000);
        }
    }

    void engineLogger() {
        std::string filename = createLogFilename();
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error abriendo archivo\n";
            return;
        }
        std::cout << "Guardando log en: " << filename << std::endl;
        std::cout << "Presione Ctrl+C para detener.\n";
        file << "timestamp,rpm,speed,coolant,load,throttle,maf\n";

        g_runningLogger = true;
        int step = 0;
        while (g_runningLogger) {
            time_t now = time(nullptr);
            int rpm      = m_elm->getRPM();
            int speed    = m_elm->getSpeed();
            int coolant  = m_elm->getCoolantTemp();
            int load     = m_elm->getEngineLoad();
            double throttle = m_elm->getThrottlePosition();
            float maf    = m_elm->getMAF();

            file << now << ","
                 << rpm << ","
                 << speed << ","
                 << coolant << ","
                 << load << ","
                 << throttle << ","
                 << maf << "\n";
            file.flush();
            step++;
            if (step % 10 == 0)
                std::cout << "Registros: " << step << "\r" << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        file.close();
        std::cout << "\nLogger detenido. " << step << " registros guardados.\n";
    }

    void detectModules() const {
        std::cout << "\n=== ESCANEO DE MODULOS CAN ===\n";
        for (int id = 0x700; id <= 0x7E7; id++) {
            std::stringstream cmd;
            cmd << "AT SH " << std::hex << id;
            m_elm->send(cmd.str());
            std::string r = m_elm->send("01 00");
            if (r.find("41") != std::string::npos) {
                std::cout << "Modulo encontrado: 0x" << std::hex << id << std::endl;
            }
            usleep(10000);
        }
        std::cout << "\nEscaneo completado\n";
    }

    void liveSensors() const {
        std::cout << "\n=== MONITOR SENSORES ===\n";
        std::cout << "Presione Ctrl+C para salir\n\n";
        while (true) {
            std::cout << "\rRPM: " << m_elm->getRPM()
                      << " | Speed: " << m_elm->getSpeed()
                      << " | MAF: " << m_elm->getMAF()
                      << " | Throttle: " << std::fixed << std::setprecision(1)
                      << m_elm->getThrottlePosition() << "%"
                      << std::flush;
            usleep(800000);
        }
    }

    void sendCustomCommand() {
        std::string customCmd;
        std::cin.ignore();
        std::cout << "Ingrese comando (ej: 22 F190): ";
        std::getline(std::cin, customCmd);
        std::string resp = m_gm->sendCommand(customCmd);
        std::cout << "Respuesta: " << resp << std::endl;
    }

    void fullSensorsLog() {
        std::string logfile = "full_sensors_log_" + std::to_string(time(nullptr)) + ".csv";
        m_elm->logAllSensorsRaw(logfile);
    }

    void p0171DiagnosticLog() {
        std::string logfile = "p0171_diagnostic_" + std::to_string(time(nullptr)) + ".csv";
        int duration = 60;
        std::cout << "Duración en segundos (default 60): ";
        std::cin >> duration;
        if (std::cin.fail()) duration = 60;
        m_elm->logP0171Diagnostic(logfile, duration);
    }

    // ---------------------------------------------------------------
    // Utilidades
    // ---------------------------------------------------------------

    // ---------------------------------------------------------------
    // Menú de reset adaptativos Chevrolet/GM
    // ---------------------------------------------------------------

    void showResetAdaptationsMenu() {
        std::cout << "\n=== RESET ADAPTATIVOS CHEVROLET/GM ===\n";
        std::cout << "1. Reset adaptativos (Método 1 - F10E)\n";
        std::cout << "2. Reset adaptativos (Método 2 - F10D)\n";
        std::cout << "3. Reset solo Fuel Trims\n";
        std::cout << "4. Reset completo (todos los métodos)\n";
        std::cout << "0. Cancelar\n";
        std::cout << "Opción: ";
        
        int opt;
        std::cin >> opt;
        
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "\nOpción inválida\n";
            return;
        }
        
        switch(opt) {
            case 1: std::cout << "\n" << m_gm->resetAdaptations() << std::endl; break;
            case 2: std::cout << "\n" << m_gm->resetAdaptations2() << std::endl; break;
            case 3: std::cout << "\n" << m_gm->resetFuelTrims() << std::endl; break;
            case 4: std::cout << "\n" << m_gm->resetAllAdaptations() << std::endl; break;
            default: std::cout << "Cancelado\n"; break;
        }
    }

    std::string createLogFilename() const {
        std::time_t now = std::time(nullptr);
        std::tm* t = std::localtime(&now);
        std::stringstream ss;
        ss << "log_" << std::put_time(t, "%Y_%m_%d_%H_%M") << ".csv";
        return ss.str();
    }
};

} // namespace OBD
