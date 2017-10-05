@ECHO OFF
ECHO Please choose build system:
ECHO 1. Visual Studio with NMake
ECHO 2. Visual Studio with Ninja Build
ECHO 3. Exit
ECHO .

CHOICE /C 123 /M "Enter your choice:"

IF ERRORLEVEL 3 GOTO End
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

:End
