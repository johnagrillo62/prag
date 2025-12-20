#
#
# Easy Reflection
#
#

#
# 1) Run metafront
#

./metafront reflect.cpp | clang-format > reflect.meta

#
# 2) Compile with reflect.meta included
#


g++ reflect.cpp -o reflect

#
# 3) Reflect
#

./reflect

#x (int): 100
#y (int): 100
#string (std::string): "reflect"
#strings (std::vector<std::string>): ["reflect", "reflect", "reflect"]
