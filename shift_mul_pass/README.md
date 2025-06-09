# Compilation
``` bash
LLVM_SRC=../llvm
LLVM_BUILD=../llvm/build
clang++ -shared -fPIC `llvm-config --cxxflags` shift_mul_pass.cpp -o shift_mul_pass.so `llvm-config --ldflags`
```
# Usage
``` bash
opt -load-pass-plugin ./shift_mul_pass.so -passes="shift-mul-pass" path/to/input.ll -S -o path/to/output.ll
```