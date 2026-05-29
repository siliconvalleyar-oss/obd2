#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>

struct OxygenSensor {
    int bank;
    int sensor;
    double voltage;
    double shortTermTrim;
};

struct DTCCode {
    std::string code;
    std::string description;
};

struct FuelTrim {
    double shortTermBank1;
    double shortTermBank2;
    double longTermBank1;
    double longTermBank2;
    bool available;
};

class ELM327
{
public:
    ELM327(const std::string& mac, int channel = 1);
    ~ELM327();

    bool connectBT();
    void disconnect();
    
    bool isConnected() { return sock >= 0; }
    bool isOnline() { return sock >= 0; }

    std::string send(const std::string& cmd, int delayMs = 200);

    // Parámetros básicos
    int getRPM();
    int getSpeed();
    int getTemp();
    int getCoolantTemp();
    int getEngineLoad();
    double getThrottlePosition();
    double getIntakePressure();
    int getIntakeTemp();
    double getTimingAdvance();
    double getFuelPressure();
    double getMAF();
    double getFuelLevel();
    double getAmbientTemp();
    double getOilTemp();
    double getCommandedEGR();
    double getEGRError();
    double getEVAPPressure();
    double getBarometricPressure();
    
    // Fuel Trim
    double getShortTermTrimBank1();
    double getShortTermTrimBank2();
    double getLongTermTrimBank1();
    double getLongTermTrimBank2();
    FuelTrim getAllFuelTrims();
    
    // Sensores de oxígeno
    std::vector<OxygenSensor> getOxygenSensors();
    OxygenSensor getO2Sensor(int bank, int sensor);
    
    // DTCs
    std::vector<DTCCode> getDTCs();
    bool clearDTCs();
    
    // Diagnóstico
    std::string getProtocol();
    std::string getVehicleInfo();
    std::string getVIN();
    bool checkMIL();
    
    bool setProtocol(int protocol);
    bool resetELM();
    std::string sendCommand(const std::string& cmd);
    
    // Nuevas funciones de logging
    void logAllSensorsRaw(const std::string& filename);
    void logP0171Diagnostic(const std::string& filename, int durationSec = 60);

private:
    int sock;
    std::string mac;
    int channel;
    
    std::string readRaw();
    std::string parseResponse(const std::string& response, const std::string& expected);
    std::string decodeDTCCode(const std::string& code);
    std::vector<std::string> splitResponse(const std::string& response);
};
