@ECHO OFF
ECHO Please choose build system:
ECHO 1. Visual Studio with NMake
ECHO 2. Ninja Build
ECHO 3. Exit
ECHO .

CHOICE /C 123 /M "Enter your choice:"

IF ERRORLEVEL 3 GOTO End
IF ERRORLEVEL 2 GOTO NinjaBuild
IF ERRORLEVEL 1 GOTO VS

:VS
mkdir buildRelease
cd buildRelease
cmake -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles" ..
cd ..
mkdir buildDebug
cd buildDebug
cmake -DCMAKE_BUILD_TYPE=Debug -G "NMake Makefiles" ..
cd ..
GOTO End

:NinjaBuild
mkdir buildRelease
cd buildRelease
cmake -DCMAKE_BUILD_TYPE=Release -G "Ninja" ..
cd .. 
mkdir buildDebug
cd buildDebug
cmake -DCMAKE_BUILD_TYPE=Debug -G "Ninja" ..
cd ..
GOTO End

:End
