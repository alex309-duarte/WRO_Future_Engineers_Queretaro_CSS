# compiler
CXX = g++

# Oradar SDK (vendored CMake project, built once into a static lib)
ORADAR_SDK_DIR = ./oradar_sdk
ORADAR_SDK_BUILD_DIR = $(ORADAR_SDK_DIR)/build
ORADAR_SDK_LIB = $(ORADAR_SDK_BUILD_DIR)/liboradar_sdk.a
ORADAR_INCLUDES = -I$(ORADAR_SDK_DIR) -I$(ORADAR_SDK_DIR)/src

CXXFLAGS = -O3 -Wall $(ORADAR_INCLUDES)
LDFLAGS = -lpthread -lgpiod

TARGET = my_lidar_app

SRCS = main.cpp spike.cpp rasp_gpio.cpp Oradar_S2L.cpp
SDK_LIB = $(ORADAR_SDK_LIB)
SDK_DEP = $(ORADAR_SDK_LIB)

all: $(TARGET)

$(ORADAR_SDK_LIB):
		cmake -S $(ORADAR_SDK_DIR) -B $(ORADAR_SDK_BUILD_DIR) -DBUILD_EXAMPLES=OFF
		cmake --build $(ORADAR_SDK_BUILD_DIR)

$(TARGET): $(SRCS) $(SDK_DEP)
		$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(SDK_LIB) $(LDFLAGS)

clean:
		rm -f $(TARGET)

clean-oradar-sdk:
		rm -rf $(ORADAR_SDK_BUILD_DIR)