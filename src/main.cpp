/**
 * @file main.cpp
 * @brief Aplicación principal para diagnóstico OBD-II con ELM327.
 * 
 * Refactorización orientada a objetos. La clase OBD::App encapsula toda la lógica,
 * mostrando un menú interactivo y ejecutando las opciones del usuario.
 */
#include "obd2_app.hpp"

#include <memory>
#include <iostream>
// ============================================================================
// Función principal
// ============================================================================
int main() {
    auto device = std::make_unique<OBD::App>();
    device->run();
    return 0;
}