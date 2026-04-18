# ============================================================================
# Makefile for PA2: k-input LUT Mapping
# ----------------------------------------------------------------------------
# Targets:
#   make all     -> compile source code and generate executable
#   make run input=<input.blif> output=<output.blif> k=<number, 2~10>
#   make clean   -> remove objects and executable
# ============================================================================

CXX       := g++
CXXFLAGS  := -std=c++17 -O0 -Wall -Iinc

TARGET    := 114521123_PA2

SRC_DIR   := src
INC_DIR   := inc
OBJ_DIR   := obj

MAIN_SRC  := 114521123_PA2.cpp
SRCS      := $(wildcard $(SRC_DIR)/*.cpp)
OBJS      := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS)) $(OBJ_DIR)/$(basename $(MAIN_SRC)).o

# ---------- build ----------
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/$(basename $(MAIN_SRC)).o: $(MAIN_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# ---------- run ----------
# Usage: make run input=<input.blif> output=<output.blif> k=<2~10>
run: all
	./$(TARGET) -input $(input) -output $(output) -k $(k)

# ---------- test ----------
TEST_SRC  := test/test_blif_parser.cpp
TEST_SRCS := $(filter-out $(SRC_DIR)/blif_writer.cpp $(SRC_DIR)/lut_eval.cpp $(SRC_DIR)/aig_builder.cpp, $(SRCS))
TEST_BIN  := test_blif_parser

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(TEST_SRCS) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# ---------- clean ----------
clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(TEST_BIN)

.PHONY: all run test clean
