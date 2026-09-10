# k-input LUT mapper
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
TARGET   := lutmap
SRCS     := $(wildcard src/*.cpp)
OBJS     := $(patsubst src/%.cpp,build/%.o,$(SRCS))

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

build/%.o: src/%.cpp inc/*.h | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

# make run input=<in.blif> output=<out.blif> k=<2..10>
run: $(TARGET)
	./$(TARGET) -input $(input) -output $(output) -k $(k)

# quick smoke test: map every testcase with k=4 into output/
test: $(TARGET)
	mkdir -p output
	@for f in testcase/*.blif; do \
	  b=$$(basename $$f .blif); \
	  ./$(TARGET) -input $$f -output output/$${b}_k4.blif -k 4 -quiet || exit 1; \
	done

clean:
	rm -rf build $(TARGET) $(TARGET).exe
