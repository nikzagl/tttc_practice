## Compiling
```sh
 clang++ -shared -fPIC $(llvm-config --cxxflags) -o var_renamer_plugin.so var_renamer_plugin.cpp $(llvm-config --ldflags) -lclangAST -lclangBasic -lclangFrontend -lclangASTMatchers
```

## Usage

``` sh
clang -fsyntax-only -Xclang -load -Xclang ./main.so   -Xclang -plugin -Xclang var-renamer path/to/file
```
(Replace path/to/file with a correct path)