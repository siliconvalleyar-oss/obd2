# Makefile para ELM327 OBD-II Diagnostic Tool
# Compilador y flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I$(INCLUDE_DIR)
LDFLAGS = -lbluetooth -pthread

# Directorios
SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = obj
BIN_DIR = bin
LOG_DIR = logs

# Archivos fuente
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
TARGET = $(BIN_DIR)/elm327_app

# Colores para output
RED = \033[0;31m
GREEN = \033[0;32m
YELLOW = \033[1;33m
NC = \033[0m # No Color

# Regla principal
all: check_dirs $(TARGET)
	@echo "$(GREEN)✅ Compilación completada$(NC)"
	@echo "$(GREEN)📁 Ejecutable: $(TARGET)$(NC)"

# Crear directorios necesarios
check_dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) $(LOG_DIR)

# Linkear el ejecutable
$(TARGET): $(OBJECTS)
	@echo "$(YELLOW)🔗 Linkeando...$(NC)"
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# Compilar objetos
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "$(YELLOW)📝 Compilando $<...$(NC)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpiar archivos compilados
clean:
	@echo "$(YELLOW)🧹 Limpiando archivos...$(NC)"
	rm -rf $(OBJ_DIR)/*.o $(TARGET)
	@echo "$(GREEN)✅ Limpieza completada$(NC)"

# Limpieza completa (incluye logs)
distclean: clean
	@echo "$(YELLOW)🗑️  Eliminando logs...$(NC)"
	rm -rf $(LOG_DIR)/*.csv $(LOG_DIR)/*.log
	@echo "$(GREEN)✅ Limpieza completa$(NC)"

# Ejecutar el programa
run: all
	@echo "$(GREEN)🚀 Ejecutando...$(NC)"
	@cd $(BIN_DIR) && ./elm327_app

# Instalar dependencias del sistema (requiere sudo)
install-deps:
	@echo "$(YELLOW)📦 Instalando dependencias del sistema...$(NC)"
	@./scripts_tools/install_deps.sh

# Verificar dependencias
check-deps:
	@echo "$(YELLOW)🔍 Verificando dependencias...$(NC)"
	@which g++ > /dev/null && echo "$(GREEN)✅ g++ encontrado$(NC)" || echo "$(RED)❌ g++ no encontrado$(NC)"
	@which make > /dev/null && echo "$(GREEN)✅ make encontrado$(NC)" || echo "$(RED)❌ make no encontrado$(NC)"
	@pkg-config --exists bluetooth && echo "$(GREEN)✅ libbluetooth-dev encontrado$(NC)" || echo "$(RED)❌ libbluetooth-dev no encontrado$(NC)"

# Ayuda
help:
	@echo "=========================================="
	@echo "   ELM327 OBD-II DIAGNOSTIC TOOL"
	@echo "=========================================="
	@echo ""
	@echo "Comandos disponibles:"
	@echo "  make all         - Compilar el programa"
	@echo "  make run         - Compilar y ejecutar"
	@echo "  make clean       - Limpiar archivos objeto"
	@echo "  make distclean   - Limpieza completa"
	@echo "  make install-deps- Instalar dependencias"
	@echo "  make check-deps  - Verificar dependencias"
	@echo "  make help        - Mostrar esta ayuda"
	@echo ""

# Reglas phony
.PHONY: all clean distclean run install-deps check-deps help check_dirs
