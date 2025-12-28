# Makefile for metafront
# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -O2 -std=c++20

# Include paths
INCLUDES = -I/mnt/c/Users/johna/source/repos/meta

# Linker flags
LDFLAGS_METAFRONT = -lclang
LDFLAGS_REFLECT = -lyaml-cpp

# Targets
METAFRONT = metafront
REFLECT = reflect

# Source files
METAFRONT_SRC = metafront.cpp
REFLECT_SRC = reflect.cpp

# Default target
all: run_reflect

# Build metafront executable
$(METAFRONT): $(METAFRONT_SRC)
	$(CXX) $(CXXFLAGS) -o $(METAFRONT) $(METAFRONT_SRC) $(LDFLAGS_METAFRONT)

# Generate reflect.meta from reflect.cpp
reflect.meta: $(REFLECT_SRC) $(METAFRONT)
	./$(METAFRONT) $(REFLECT_SRC) > reflect.meta

# Build reflect executable
$(REFLECT): $(REFLECT_SRC) reflect.meta
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(REFLECT) $(REFLECT_SRC) $(LDFLAGS_REFLECT)

# Run reflect
run_reflect: $(REFLECT)
	./$(REFLECT)

# Clean up build artifacts
clean:
	rm -f $(METAFRONT) $(REFLECT) reflect.meta

# Phony targets
.PHONY: all run_reflect clean

