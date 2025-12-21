# Tools
CXX := g++
CXXFLAGS := -std=c++20
REFLECT := ../../../build/out/cpp-reflect

# Directories
BIN_DIR := ./bin

# Find files
HEADERS := $(wildcard *.h)
CPPS := $(wildcard *.cpp)

# Targets
METAS := $(HEADERS:.h=.meta)
BINS := $(addprefix $(BIN_DIR)/,$(CPPS:.cpp=))

# Build everything
.PHONY: all
all: $(METAS) $(BINS)

# Generate .meta from .h
%.meta: %.h
	$(REFLECT) $< > $@

# Compile .cpp to executable
$(BIN_DIR)/%: %.cpp | $(BIN_DIR)
	$(CXX) $< -o $@ $(CXXFLAGS)

# Create bin directory
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Clean
.PHONY: clean
clean:
	rm -f *.meta
	rm -rf $(BIN_DIR)
