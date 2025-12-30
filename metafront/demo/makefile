# ------------------------------------------------------------
# Compiler + flags
# ------------------------------------------------------------
CXX = g++ -I/mnt/c/Users/johna/source/repos/metah
CXXFLAGS = -Wall -Wextra -std=c++20 \
           -Wno-missing-field-initializers \
           -Wno-attributes \
           -Wno-unused-parameter

LDFLAGS = -lyaml-cpp

# ------------------------------------------------------------
# Tools
# ------------------------------------------------------------
METAFRONT = ../metafront
ASTREI    = ../../../astrei/parser

# ------------------------------------------------------------
# Demo numbers
# ------------------------------------------------------------
DEMOS = 01 02 03 04 05 06 07 08 09 10 11 12 14 all

# Generated names
META_DEMOS = $(addprefix demo, $(DEMOS))
META_FILES = $(addsuffix .meta, $(META_DEMOS))

# ------------------------------------------------------------
# Default target
# ------------------------------------------------------------
all: metafront

# ------------------------------------------------------------
# METAFRONT DEMOS
# ------------------------------------------------------------
metafront: $(META_DEMOS)

demo%: demo%.cpp
	@echo "Processing demo$* (metafront)..."
	@cat demo$*.cpp
	@$(METAFRONT) demo$*.cpp | clang-format > demo$*.meta
	@echo "**************"
	@$(CXX) $(CXXFLAGS) demo$*.cpp -o bin/demo$* $(LDFLAGS)
	@./bin/demo$*
	@echo ""

# ------------------------------------------------------------
# CLEAN
# ------------------------------------------------------------
clean:
	rm -f $(META_FILES) $(META_DEMOS)

.PHONY: all clean metafront


