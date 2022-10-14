# HF CSV
HF CSV is a simple single-header csv utility written in C. It allows you to create, read, edit, and save csv files. Not supposed to be anything serious, more of a fun project.

# Build
Built using CMake, tested with MingW. There's a simple test program at main.c, where the library itself is included and its implementation defined.

# Usage
To use the library, include hf_csv.h. In one of your source files, HF_CSV_IMPLEMENTATION should be defined before including, so that the implementation is then defined there, like you'd do with a stb header.

# TODO:
- Complete Usage section of readme
