# Building OpenPnP Capture Library

## Windows Build Options

The bootstrap.bat script now provides multiple build options for Windows:

### Using bootstrap.bat

Run `bootstrap.bat` and choose from the following options:

1. **Visual Studio with NMake (Shared Library - Release)** - Default shared library build
2. **Visual Studio with Ninja Build (Shared Library - Release)** - Shared library with Ninja
3. **Visual Studio with NMake (Static Library - Release)** - Static library, Release mode
4. **Visual Studio with NMake (Static Library - Debug)** - Static library, Debug mode  
5. **Visual Studio with Ninja Build (Static Library - Release)** - Static library with Ninja, Release
6. **Visual Studio with Ninja Build (Static Library - Debug)** - Static library with Ninja, Debug
7. **Exit**

### Manual CMake Configuration

You can also configure the build manually using CMake options:

#### For Shared Library (default):
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles" ..
```

#### For Static Library:
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -G "NMake Makefiles" ..
```

#### For Debug Static Library:
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -G "NMake Makefiles" ..
```

## CMake Options

- `BUILD_SHARED_LIBS`: Build shared library (default: ON)
- `BUILD_STATIC_LIBS`: Build static library (default: OFF)

**Note**: If both options are enabled, the build will default to static library with a warning.

## Usage in Your Project

### When Using Static Library

When linking against the static library, make sure to define `OPENPNPCAPTURE_STATIC` in your project to ensure correct symbol linkage:

```cpp
#define OPENPNPCAPTURE_STATIC
#include "openpnp-capture.h"
```

Or add it as a compile definition in your CMakeLists.txt:

```cmake
target_compile_definitions(your_target PRIVATE OPENPNPCAPTURE_STATIC)
```

### When Using Shared Library

No special definitions are needed for shared library usage:

```cpp
#include "openpnp-capture.h"
```
