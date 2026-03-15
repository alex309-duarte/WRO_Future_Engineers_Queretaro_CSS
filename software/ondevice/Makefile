# compiler
CXX = g++

# SDK Directory
SDK_DIR = ./rplidar_sdk
SDK_LIB = $(SDK_DIR)/output/Linux/Release/libsl_lidar_sdk.a

# Includes and Flags
INCLUDES = -I$(SDK_DIR)/sdk/include -I$(SDK_DIR)/sdk/src
CXXFLAGS = -O3 -Wall $(INCLUDES)
LDFLAGS = -lpthread

# Source and Target
TARGET = my_lidar_app
SRCS = spike.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
		$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(SDK_LIB) $(LDFLAGS)

clean:
		rm -f $(TARGET)