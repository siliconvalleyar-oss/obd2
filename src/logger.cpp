#include "logger.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>

Logger::Logger()
{
}

Logger::~Logger()
{
    close();
}

bool Logger::open()
{
    time_t now = time(nullptr);
    tm* t = localtime(&now);

    std::stringstream ss;

    ss << "log_"
       << (t->tm_year + 1900)
       << std::setw(2) << std::setfill('0') << (t->tm_mon + 1)
       << std::setw(2) << std::setfill('0') << t->tm_mday
       << "_"
       << std::setw(2) << std::setfill('0') << t->tm_hour
       << ":"
       << std::setw(2) << std::setfill('0') << t->tm_min;

    filename = ss.str();

    file = std::make_unique<std::ofstream>(filename);

    if (!file->is_open())
        return false;

    *file << "timestamp,key,value\n";

    return true;
}


void Logger::log(const std::string& key, const std::string& value)
{
    if (!file || !file->is_open()) return;
    time_t now = time(nullptr);
    *file << now << "," << key << "," << value << std::endl;
    file->flush();  // Forzar escritura inmediata
}

void Logger::log(const std::string& key, int value)
{
    log(key, std::to_string(value));
}

void Logger::log(const std::string& key, float value)
{
    log(key, std::to_string(value));
}

void Logger::log(const std::string& key, double value)
{
    log(key, std::to_string(value));
}

void Logger::close()
{
    if (file && file->is_open())
    {
        file->flush();
        file->close();
    }
}