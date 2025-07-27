@ECHO ON

set BASEDIR=%~dp0
PUSHD %BASEDIR%

RMDIR /Q /S build_release

conan install . --output-folder=build_release --build=missing -s build_type=Release 
cd build_release
cmake .. -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_PREFIX_PATH="D:\Program Files\eProsima\fastrtps 2.11.3;d:\local" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
