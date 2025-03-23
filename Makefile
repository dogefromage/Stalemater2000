CC = g++
CC_FLAGS = -Wpedantic -Wall -Wextra -O1 -I./include -std=c++20
# CC_FLAGS = -Wpedantic -Wall -Wextra -g -I./include -std=c++20
LINK_FLAGS =

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

CPP_SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(CPP_SRC_FILES))

EXEC_NAME = stalemater
TARGET = $(BIN_DIR)/$(EXEC_NAME)

all: $(TARGET)

$(BUILD_DIR)/nnue_data.o: $(SRC_DIR)/nnue_data.s
	@mkdir -p $(BUILD_DIR)
	as -o $(BUILD_DIR)/nnue_data.o $(SRC_DIR)/nnue_data.s

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CC_FLAGS) -c $< -o $@

$(TARGET): $(BUILD_DIR)/nnue_data.o $(OBJ_FILES)
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -o $@ $(LINK_FLAGS)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean
