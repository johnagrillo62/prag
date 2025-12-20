# Makefile for metafront

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -O2

# Linker flags
LDFLAGS = -lclang

# Target executable
TARGET = metafront reflect

# Source file
SRC = metafront.cpp

# Default target
all: run_reflect 

# Build the executable
metafont: $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

reflect.meta: reflect.cpp metafront
	./metafront reflect.cpp > reflect.meta
	$(CXX) reflect.cpp -o reflect $(LDFLAGS)

run_reflect: metafront reflect.meta
	./reflect

# Clean up build artifacts
clean:
	rm -f metafront

# Phony targets
.PHONY: all clean
