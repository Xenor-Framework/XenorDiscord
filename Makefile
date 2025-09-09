CXX := g++
CXXFLAGS := -Wall -O2 -std=c++17 -m32 -fPIC
LDFLAGS := -shared -m32 -lcurl

INCLUDES := -Iinclude

SRC := src/gm_discordgateway.cpp
OBJ := build/$(notdir $(SRC:.cpp=.o))

TARGET := lib/gmsv_discordgateway_linux.dll.so

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -o $@ $^

build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf build
	rm -rf lib

.PHONY: all clean
