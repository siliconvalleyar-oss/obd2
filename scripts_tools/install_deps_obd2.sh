#!/bin/bash

# ============================================
# Script de instalación de dependencias
# para ELM327 OBD-II Diagnostic Tool
# ============================================

set -e  # Salir si hay error

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Detectar distribución
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO=$ID
        DISTRO_VERSION=$VERSION_ID
    elif command -v lsb_release >/dev/null 2>&1; then
        DISTRO=$(lsb_release -si | tr '[:upper:]' '[:lower:]')
        DISTRO_VERSION=$(lsb_release -sr)
    else
        DISTRO="unknown"
    fi
    echo "$DISTRO"
}

# Obtener comando de instalación según la distribución
get_install_cmd() {
    case "$1" in
        ubuntu|debian|linuxmint|pop)
            echo "apt-get install -y"
            ;;
        fedora|rhel|centos)
            echo "dnf install -y"
            ;;
        arch|manjaro)
            echo "pacman -S --noconfirm"
            ;;
        opensuse*)
            echo "zypper install -y"
            ;;
        *)
            echo "unknown"
            ;;
    esac
}

# Instalar paquetes según distribución
install_packages() {
    local distro=$1
    local cmd=$(get_install_cmd "$distro")
    
    if [ "$cmd" = "unknown" ]; then
        echo -e "${RED}❌ Distribución no soportada para instalación automática${NC}"
        echo -e "${YELLOW}Por favor, instala manualmente:${NC}"
        echo "  - build-essential / base-devel"
        echo "  - libbluetooth-dev / bluez-libs-devel"
        echo "  - cmake, git"
        return 1
    fi
    
    echo -e "${BLUE}📦 Instalando paquetes para $distro...${NC}"
    
    case "$distro" in
        ubuntu|debian|linuxmint|pop)
            sudo apt-get update
            sudo $cmd build-essential cmake git
            sudo $cmd libbluetooth-dev
            sudo $cmd bluetooth bluez bluez-tools
            sudo $cmd libssl-dev  # Opcional, para comunicaciones seguras
            ;;
        fedora|rhel|centos)
            sudo $cmd gcc-c++ make cmake git
            sudo $cmd bluez-libs-devel
            sudo $cmd bluez bluez-tools
            sudo $cmd openssl-devel
            ;;
        arch|manjaro)
            sudo $cmd base-devel cmake git
            sudo $cmd bluez bluez-libs
            sudo $cmd bluez-utils
            ;;
        opensuse*)
            sudo $cmd gcc-c++ make cmake git
            sudo $cmd bluez-devel
            sudo $cmd bluez bluez-auto-enable-devices
            ;;
    esac
}

# Configurar Bluetooth (opcional)
setup_bluetooth() {
    echo -e "${BLUE}🔧 Configurando Bluetooth...${NC}"
    
    # Habilitar y iniciar servicio Bluetooth si existe
    if command -v systemctl >/dev/null 2>&1; then
        sudo systemctl enable bluetooth 2>/dev/null || true
        sudo systemctl start bluetooth 2>/dev/null || true
        echo -e "${GREEN}✅ Servicio Bluetooth habilitado${NC}"
    fi
    
    # Verificar que el usuario esté en el grupo bluetooth
    if groups | grep -q bluetooth; then
        echo -e "${GREEN}✅ Usuario ya está en grupo bluetooth${NC}"
    else
        echo -e "${YELLOW}⚠️  Agregando usuario al grupo bluetooth...${NC}"
        sudo usermod -a -G bluetooth "$USER"
        echo -e "${YELLOW}⚠️  Puede que necesites cerrar sesión y volver a entrar para aplicar cambios${NC}"
    fi
}

# Verificar instalaciones
verify_installations() {
    echo -e "\n${BLUE}🔍 Verificando instalaciones...${NC}"
    
    # Verificar compilador
    if command -v g++ >/dev/null 2>&1; then
        echo -e "${GREEN}✅ g++ $(g++ --version | head -n1)${NC}"
    else
        echo -e "${RED}❌ g++ no encontrado${NC}"
    fi
    
    # Verificar make
    if command -v make >/dev/null 2>&1; then
        echo -e "${GREEN}✅ make $(make --version | head -n1)${NC}"
    else
        echo -e "${RED}❌ make no encontrado${NC}"
    fi
    
    # Verificar cmake
    if command -v cmake >/dev/null 2>&1; then
        echo -e "${GREEN}✅ cmake $(cmake --version | head -n1)${NC}"
    else
        echo -e "${YELLOW}⚠️  cmake no instalado (opcional)${NC}"
    fi
    
    # Verificar Bluetooth
    if [ -f /usr/include/bluetooth/bluetooth.h ] || [ -f /usr/include/bluetooth.h ]; then
        echo -e "${GREEN}✅ libbluetooth headers encontrados${NC}"
    else
        echo -e "${RED}❌ libbluetooth headers no encontrados${NC}"
        return 1
    fi
    
    # Verificar servicio Bluetooth
    if command -v hciconfig >/dev/null 2>&1; then
        echo -e "${GREEN}✅ bluez-utils instalado${NC}"
    else
        echo -e "${YELLOW}⚠️  bluez-utils no instalado (hciconfig no disponible)${NC}"
    fi
}

# Función principal
main() {
    echo -e "${GREEN}=========================================="
    echo -e "   INSTALADOR DE DEPENDENCIAS"
    echo -e "   ELM327 OBD-II Diagnostic Tool"
    echo -e "==========================================${NC}\n"
    
    # Detectar distribución
    DISTRO=$(detect_distro)
    echo -e "${BLUE}📋 Distribución detectada: $DISTRO${NC}\n"
    
    # Instalar paquetes
    if install_packages "$DISTRO"; then
        echo -e "\n${GREEN}✅ Paquetes instalados correctamente${NC}"
    else
        echo -e "\n${RED}❌ Error instalando paquetes${NC}"
        exit 1
    fi
    
    # Configurar Bluetooth
    setup_bluetooth
    
    # Verificar instalación
    verify_installations
    
    echo -e "\n${GREEN}=========================================="
    echo -e "✅ INSTALACIÓN COMPLETADA"
    echo -e "==========================================${NC}"
    
    echo -e "\n${YELLOW}Para compilar el programa:${NC}"
    echo -e "  make all"
    echo -e "\n${YELLOW}Para ejecutar:${NC}"
    echo -e "  make run"
    echo -e "  cd bin && ./elm327_app"
    
    echo -e "\n${YELLOW}Nota:${NC} Si agregaste el usuario al grupo bluetooth,"
    echo -e "debes cerrar sesión y volver a entrar para que los cambios surtan efecto."
}

# Ejecutar main
main "$@"
