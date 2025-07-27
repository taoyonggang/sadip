# [proj-begin]
cmake_minimum_required(VERSION 3.22)

# 设置 CMake policy CMP0091 为 NEW
cmake_policy(SET CMP0091 NEW)

project(sadip)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows 特定设置
if(WIN32)
    add_definitions(
        -D_WIN32_WINNT=0x0601
        -DWIN32_LEAN_AND_MEAN
        -DNOMINMAX
    )
    
    # 如果使用 MSVC 编译器
    if(MSVC)
        # 添加额外的编译选项
        add_compile_options(
            /W4     # 警告级别
            /MP     # 多处理器编译
            /EHsc   # 标准 C++ 异常处理
        )
    endif()
endif()

# 打印构建信息
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "CONAN_LIBS: ${CONAN_LIBS}")

# 确保使用conan工具链
include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
	MESSAGE(STATUS "current platform: Linux ")
	#include(/usr/local/vcpkg/scripts/buildsystems/vcpkg.cmake)
        include_directories(/usr/include/mysql)
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
	MESSAGE(STATUS "current platform: Windows")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	MESSAGE(STATUS "current platform: FreeBSD")
ELSE ()
	MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

find_package(OpenSSL REQUIRED)

find_package(CppMicroServices REQUIRED)

find_package(SOCI CONFIG REQUIRED)


find_package(spdlog CONFIG REQUIRED)




include(GenerateExportHeader)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
#include_directories(${VCPKG_INSTALL}/include)
include_directories(third_party)
include_directories(third_party/concurrentqueue)
include_directories(db)
include_directories(db/rd)
include_directories(utils)
include_directories(dds)

function(GenerateService _name _args)
    set(_srcs ${ARGN})

    usFunctionGetResourceSource(TARGET ${_name} OUT _srcs)
    usFunctionGenerateBundleInit(TARGET ${_name} OUT _srcs)

    add_library(${_name} SHARED ${_srcs})
    set_property(TARGET ${_name} APPEND PROPERTY COMPILE_DEFINITIONS US_BUNDLE_NAME=${_name})
    set_property(TARGET ${_name} PROPERTY DEBUG_POSTFIX "")

    if(${_name}_DEPENDS)
        foreach(_dep ${${_name}_DEPENDS})
            include_directories(${PROJECT_SOURCE_DIR}/${_dep})
            target_link_libraries(${_name} ${_dep})
        endforeach()
    endif()
    target_link_libraries(${_name} ${CppMicroServices_LIBRARIES} spdlog::spdlog)
    set_target_properties(${_name} PROPERTIES
            OUTPUT_NAME ${_name}
            )

    usFunctionAddResources(TARGET ${_name} BUNDLE_NAME ${_name} WORKING_DIRECTORY ../resources FILES manifest.json)
    usFunctionEmbedResources(TARGET ${_name})

endfunction()

add_subdirectory(node)
#add_subdirectory(databus)
#add_subdirectory(mms)
#add_subdirectory(clock)
#add_subdirectory(consumer)
#add_subdirectory(cam-edge)
#add_subdirectory(cam-cloud)
#add_subdirectory(cam-app)
#add_subdirectory(syncdb-app)
add_subdirectory(monitor-app)
#add_subdirectory(log-app)
#add_subdirectory(hello-app)
#add_subdirectory(general-database)
