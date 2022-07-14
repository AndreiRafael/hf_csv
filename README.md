# HF CSV
HF CSV is a simple csv utility written in C. It allows you to load, read/edit, and save csv files. Not supposed to be anything serious, more of a fun project.

# Build
Build tested with CMake and MingW. There's a simple test program, and the library itself is built as a dll, though it should be trivial to add hf_csv.c to a build with cmake if dll is not desired.

# TODO:
- Wrap everything in extern "C" and test C++ support.
- Add some checks for failed memory allocations??
- Review actual parsing part of the code, it's a bit of a mess with some unnecessarily repeated code.
- Add Usage section to readme
