# compiler
CXX = g++

# Which lidar to build for: rplidar (default) or oradar
# usage: make            -> RPLidar S2L (unchanged default behaviour)
#        make LIDAR=oradar -> Oradar MS200/MS2000
LIDAR ?= rplidar

# RPLidar SDK (prebuilt static lib)
RPLIDAR_SDK_DIR = ./rplidar_sdk
RPLIDAR_SDK_LIB = $(RPLIDAR_SDK_DIR)/output/Linux/Release/libsl_lidar_sdk.a
RPLIDAR_INCLUDES = -I$(RPLIDAR_SDK_DIR)/sdk/include -I$(RPLIDAR_SDK_DIR)/sdk/src

# Oradar SDK (vendored CMake project, built once into a static lib)
ORADAR_SDK_DIR = ./oradar_sdk
ORADAR_SDK_BUILD_DIR = $(ORADAR_SDK_DIR)/build
ORADAR_SDK_LIB = $(ORADAR_SDK_BUILD_DIR)/liboradar_sdk.a
ORADAR_INCLUDES = -I$(ORADAR_SDK_DIR) -I$(ORADAR_SDK_DIR)/src

CXXFLAGS = -O3 -Wall
LDFLAGS = -lpthread -lgpiod

TARGET = my_lidar_app

ifeq ($(LIDAR),oradar)
	SRCS = main.cpp spike.cpp rasp_gpio.cpp Oradar_S2L.cpp
	CXXFLAGS += -DUSE_ORADAR $(ORADAR_INCLUDES)
	SDK_LIB = $(ORADAR_SDK_LIB)
	SDK_DEP = $(ORADAR_SDK_LIB)
else
	SRCS = main.cpp spike.cpp rasp_gpio.cpp RPLidar_S2L.cpp
	CXXFLAGS += $(RPLIDAR_INCLUDES)
	SDK_LIB = $(RPLIDAR_SDK_LIB)
	SDK_DEP =
endif

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