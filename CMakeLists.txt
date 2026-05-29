cmake_minimum_required(VERSION 3.10)
project(ELM327_BT)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Establecer el directorio de salida para el ejecutable FUERA de build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)

find_package(Threads REQUIRED)

include_directories(include)

#add_executable(elm327_app
#    src/main.cpp
#    src/gm_commands.cpp
#    src/elm327.cpp
#    src/logger.cpp
#)

file(GLOB SOURCES
    src/*.cpp
)

add_executable(elm327_app
    ${SOURCES}
)

target_link_libraries(elm327_app bluetooth Threads::Threads)

# Mensaje informativo
message(STATUS "El ejecutable se generará en: ${CMAKE_SOURCE_DIR}/bin")