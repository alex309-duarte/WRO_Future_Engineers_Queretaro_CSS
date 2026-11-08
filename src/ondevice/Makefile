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
HIWONDER_SRCS = spike_hiwonder.cpp parada_emergencia.cpp $(HIWONDER_DIR)/src/hiwonder_brick.cpp spike_compat.cpp
# La calibracion vive en headers: sin esto, make no recompila al cambiarla.
HIWONDER_HEADERS = spike.h hiwonder_config.h parada_emergencia.h $(wildcard $(HIWONDER_DIR)/include/*/*)

# Lidar del robot: RPLIDAR_S2L (por defecto) u ORADAR (MS200).
#   make WRO_LIDAR=ORADAR
# La orientacion del S2L se ajusta con LIDAR_FLAGS, p. ej.
#   make LIDAR_FLAGS="-DRPLIDAR_FRONT_DEG=180"
WRO_LIDAR ?= RPLIDAR_S2L
RPLIDAR_SDK_DIR ?= $(HOME)/rplidar_sdk
RPLIDAR_INCLUDES = -I$(RPLIDAR_SDK_DIR)/sdk/include -I$(RPLIDAR_SDK_DIR)/sdk/src
RPLIDAR_LIB = $(RPLIDAR_SDK_DIR)/output/Linux/Release/libsl_lidar_sdk.a

ifeq ($(WRO_LIDAR),RPLIDAR_S2L)
LIDAR_INCLUDES = $(RPLIDAR_INCLUDES)
LIDAR_LIB = $(RPLIDAR_LIB)
else ifeq ($(WRO_LIDAR),ORADAR)
LIDAR_INCLUDES = $(ORADAR_INCLUDES)
LIDAR_LIB = $(ORADAR_SDK_LIB)
else
$(error WRO_LIDAR debe ser RPLIDAR_S2L u ORADAR)
endif

CXXFLAGS = -O3 -Wall -std=gnu++17 $(LIDAR_INCLUDES) -I$(HIWONDER_DIR)/include -DWRO_LIDAR_$(WRO_LIDAR) $(LIDAR_FLAGS)
LDFLAGS = -lpthread -lgpiod

# Ventana del lidar (lidar_display.cpp, se apaga con --no-display): OpenCV.
OPENCV_CFLAGS = $(shell pkg-config --cflags opencv4)
OPENCV_LIBS = $(shell pkg-config --libs opencv4)

TARGET = my_lidar_app

SRCS = main.cpp $(HIWONDER_SRCS) rasp_gpio.cpp Oradar_S2L.cpp lidar_display.cpp
SDK_LIB = $(LIDAR_LIB)
SDK_DEP = $(LIDAR_LIB)

all: $(TARGET)

# Recompila al cambiar WRO_LIDAR o LIDAR_FLAGS.
LIDAR_STAMP = .lidar_$(WRO_LIDAR)$(subst =,_,$(subst -D,,$(subst $() ,,$(LIDAR_FLAGS))))
$(LIDAR_STAMP):
		rm -f .lidar_*
		touch $@

$(ORADAR_SDK_LIB):
		cmake -S $(ORADAR_SDK_DIR) -B $(ORADAR_SDK_BUILD_DIR) -DBUILD_EXAMPLES=OFF
		cmake --build $(ORADAR_SDK_BUILD_DIR)

$(TARGET): $(SRCS) $(HIWONDER_HEADERS) Oradar_S2L.h lidar_display.h $(SDK_DEP) $(LIDAR_STAMP)
		$(CXX) $(CXXFLAGS) $(OPENCV_CFLAGS) $(SRCS) -o $(TARGET) $(SDK_LIB) $(LDFLAGS) $(OPENCV_LIBS)

clean:
		rm -f $(TARGET) $(TEST_BIN) .lidar_*

# Pruebas offline del adaptador contra una placa falsa (no abren el puerto).
TEST_BIN = tests/spike_hiwonder_test
TEST_SRCS = tests/spike_hiwonder_test.cpp tests/fake_brick.cpp spike_hiwonder.cpp spike_compat.cpp

test: $(TEST_BIN)
		./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRCS) tests/fake_brick.h $(HIWONDER_HEADERS)
		$(CXX) $(CXXFLAGS) -Wextra $(TEST_SRCS) -o $(TEST_BIN) -lpthread

clean-oradar-sdk:
		rm -rf $(ORADAR_SDK_BUILD_DIR)