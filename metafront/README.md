# Metafront - C++ Simulator for C++ Reflection

Metafront Tuple Extract

## Overview

Metafront parses C++ header files and generates tuple to support c++ reflection.

## Build Requirements

- **Clang/LLVM libraries** - Required for C++ AST parsing
- My os is "Ubuntu 24.04.2 LTS" (WSL)
- Hope to release as an exe like clang-format
-  Can anyboyd help with that?

## Building

g++ metafront.cpp -o metafront `llvm-config --cxxflags --ldflags --libs --system-libs` -lclang -fpermissive`
### You will need to adjust the args in metafront.cpp

    const char* args[] = {"-std=c++20",
                          "-x",
                          "c++-header",
                          "-fno-delayed-template-parsing",
                          "-I.",
                          "-Ibhw",
                          "-isystem/usr/include",
                          "-isystem/usr/include/x86_64-linux-gnu",
                          "-isystem/usr/local/include"};

#

# metafront reflect.h > reflect.meta
# g++ reflect.cpp -o reflect
# ./reflect


x (int): 100
y (int): 100
string (std::string): "reflect"
strings (std::vector<std::string>): ["reflect", "reflect", "reflect"]


