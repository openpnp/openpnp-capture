@ECHO OFF
ECHO Please choose build system:
ECHO 1. Visual Studio with NMake (Shared Library - Release)
ECHO 2. Visual Studio with Ninja Build (Shared Library - Release)
ECHO 3. Visual Studio with NMake (Static Library - Release)
ECHO 4. Visual Studio with NMake (Static Library - Debug)
ECHO 5. Visual Studio with Ninja Build (Static Library - Release)
ECHO 6. Visual Studio with Ninja Build (Static Library - Debug)
ECHO 7. Exit
ECHO .

CHOICE /C 1234567 /M "Enter your choice:"

IF ERRORLEVEL 7 GOTO End
IF ERRORLEVEL 6 GOTO NinjaStaticDebug
IF ERRORLEVEL 5 GOTO NinjaStaticRelease
IF ERRORLEVEL 4 GOTO VSStaticDebug
IF ERRORLEVEL 3 GOTO VSStaticRelease
IF ERRORLEVEL 2 GOTO NinjaBuild
IF ERRORLEVEL 1 GOTO VS

:VS
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles" ..
cd ..
GOTO End

:NinjaBuild
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -G "Ninja" ..
cd .. 
GOTO End

:VSStaticRelease
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -G "NMake Makefiles" ..
cd ..
GOTO End

:VSStaticDebug
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -G "NMake Makefiles" ..
cd ..
GOTO End

:NinjaStaticRelease
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -G "Ninja" ..
cd ..
GOTO End

:NinjaStaticDebug
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -G "Ninja" ..
cd ..
GOTO End

:End
