@ECHO ON

set BASEDIR=%~dp0
PUSHD %BASEDIR%

RMDIR /Q /S build_debug
MKDIR build_debug

conan install . --output-folder=build_debug --build=missing -s build_type=Debug 
cd build_debug
cmake .. -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_PREFIX_PATH="c:\local;c:\local\debug" -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
