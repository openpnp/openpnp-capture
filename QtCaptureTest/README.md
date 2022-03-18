# Dependencies
- [Qt 4+](https://www.qt.io/).
- C++ compiler.
- make


# Build
```
qmake
make
```

# macOS
To run on macOS I had to include the build path for openpnp-capture:
```
> DYLD_LIBRARY_PATH=../build ./QtCaptureTest
```
