#!/usr/bin/bash env

echo "-I/usr/lib/clang/$(llvm-config --version)/include" >> compile_flags.txt
exit 0
