# Compilador
CXX = g++

# Bibliotecas
LIBS = -lssl -lcrypto -lsecp256k1

# Flags de otimização específicas para criptografia
CRYPTO_FLAGS = \
    -O3 \
    -march=native \
    -maes \
    -msse4.2 \
    -mavx2

# Flags de performance para operações matemáticas e memória
PERFORMANCE_FLAGS = \
    -ffast-math \
    -funroll-loops \
    -fomit-frame-pointer \
    -fmerge-all-constants \
    -fno-schedule-insns \
    -ftree-vectorize \
    -fno-trapping-math \
    -pipe

# Link Time Optimization
LTO_FLAGS = -flto=auto -fuse-linker-plugin

# Combinação de todas as flags
CXXFLAGS = $(CRYPTO_FLAGS) $(PERFORMANCE_FLAGS) $(LTO_FLAGS)

# Arquivos fonte e destino
TARGET = teste
SRC = teste.cpp

# Alvo principal
all: release

# Build otimizado
release:
	@echo "Compilando versão otimizada..."
	$(CXX) -o $(TARGET) $(SRC) $(CXXFLAGS) $(LIBS)
	@echo "Compilação concluída!"
	@echo "Execute com: ./$(TARGET)"

# Build para debug
debug:
	$(CXX) -o $(TARGET)_debug $(SRC) -g -O0 -Wall -Wextra $(LIBS)

# Limpar
clean:
	rm -f $(TARGET) $(TARGET)_debug

.PHONY: all release debug clean