

# Compiler
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20 -Wno-missing-field-initializers -Wno-attributes -Wno-unused-parameter

# Tools
METAFRONT = ../metafront
ASTREI = ../../../astrei/parser

# Demo numbers
DEMOS = 01 02 03 04 05 06 07 08 09 10 all

# Generate target names
META_DEMOS = $(addprefix demo, $(DEMOS))

# Generated meta files
META_FILES = $(addsuffix .meta, $(META_DEMOS))

# Default: build and run both sets
all: metafront 

# -------- METAFRONT DEMOS --------
metafront: $(META_DEMOS)

demo%: demo%.cpp
	@echo "Processing demo$* (metafront)..."
	@cat demo$*.cpp
	@$(METAFRONT) demo$*.cpp | clang-format > demo$*.meta
	@echo "**************"
	@$(CXX) $(CXXFLAGS) demo$*.cpp -o demo$*
	@./demo$*
	@rm demo$*
	@echo ""


# -------- CLEAN --------
clean:
	rm -f $(META_FILES) $(AST_FILES) $(META_DEMOS) $(AST_DEMOS)

.PHONY: all clean metafront astrei
