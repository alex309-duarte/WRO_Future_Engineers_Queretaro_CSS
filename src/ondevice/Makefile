# compiler
CXX = g++

# Oradar SDK (vendored CMake project, built once into a static lib)
ORADAR_SDK_DIR = ./oradar_sdk
ORADAR_SDK_BUILD_DIR = $(ORADAR_SDK_DIR)/build
ORADAR_SDK_LIB = $(ORADAR_SDK_BUILD_DIR)/liboradar_sdk.a
ORADAR_INCLUDES = -I$(ORADAR_SDK_DIR) -I$(ORADAR_SDK_DIR)/src

# Placa Hiwonder (rama reto1-hiwonder): spike_hiwonder.cpp y spike_compat
# implementan la API SPIKE del equipo sobre la libreria hiwonder_brick, copiada
# en ./hiwonder. spike.cpp (driver del REPL del hub) ya no se compila.
HIWONDER_DIR = ./hiwonder
HIWONDER_SRCS = spike_hiwonder.cpp $(HIWONDER_DIR)/src/hiwonder_brick.cpp $(HIWONDER_DIR)/src/spike_compat.cpp
# La calibracion vive en headers: sin esto, make no recompila al cambiarla.
HIWONDER_HEADERS = spike.h hiwonder_config.h $(wildcard $(HIWONDER_DIR)/include/*/*)

CXXFLAGS = -O3 -Wall -std=gnu++17 $(ORADAR_INCLUDES) -I$(HIWONDER_DIR)/include
LDFLAGS = -lpthread -lgpiod

TARGET = my_lidar_app

SRCS = main.cpp $(HIWONDER_SRCS) rasp_gpio.cpp Oradar_S2L.cpp
SDK_LIB = $(ORADAR_SDK_LIB)
SDK_DEP = $(ORADAR_SDK_LIB)

all: $(TARGET)

$(ORADAR_SDK_LIB):
		cmake -S $(ORADAR_SDK_DIR) -B $(ORADAR_SDK_BUILD_DIR) -DBUILD_EXAMPLES=OFF
		cmake --build $(ORADAR_SDK_BUILD_DIR)

$(TARGET): $(SRCS) $(HIWONDER_HEADERS) $(SDK_DEP)
		$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(SDK_LIB) $(LDFLAGS)

clean:
		rm -f $(TARGET) $(TEST_BIN)

# Pruebas offline del adaptador contra una placa falsa (no abren el puerto).
TEST_BIN = tests/spike_hiwonder_test
TEST_SRCS = tests/spike_hiwonder_test.cpp tests/fake_brick.cpp spike_hiwonder.cpp $(HIWONDER_DIR)/src/spike_compat.cpp

test: $(TEST_BIN)
		./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRCS) tests/fake_brick.h $(HIWONDER_HEADERS)
		$(CXX) $(CXXFLAGS) -Wextra $(TEST_SRCS) -o $(TEST_BIN) -lpthread

clean-oradar-sdk:
		rm -rf $(ORADAR_SDK_BUILD_DIR)