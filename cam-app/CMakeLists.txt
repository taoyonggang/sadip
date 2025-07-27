

##
# [proj-begin]
cmake_minimum_required(VERSION 3.22)

# 设置 CMake policy CMP0091 为 NEW
cmake_policy(SET CMP0091 NEW)

set (_lib_name cam-app)
project(${_lib_name} VERSION 1 LANGUAGES CXX)

# project(sadip)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLIBRDKAFKA_STATICLIB ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX ")

# 强调所有目标使用动态运行时库
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")



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



# ## 选定本机protobuf-3.8.0版本
# # 添加 Protobuf 路径
# set(Protobuf_ROOT "F:/protobuf_install")
# set(Protobuf_INCLUDE_DIR "F:/protobuf_install/include")
# set(Protobuf_LIB_DIR "F:/protobuf_install/lib")



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

# 查找 Protobuf
find_package(Protobuf REQUIRED)
if (Protobuf_FOUND)
    message(STATUS "Found Protobuf Version: ${Protobuf_VERSION}")
    message(STATUS "Protobuf Include Path: ${Protobuf_INCLUDE_DIR}")
    message(STATUS "Protobuf Library Path: ${Protobuf_LIB_DIR}")
else()
    message(FATAL_ERROR "Protobuf not found!")
endif()

find_package(CppMicroServices REQUIRED)
find_package(Boost REQUIRED)
find_package(zenohc) 
find_package(zenohcxx)
find_package(OpenSSL REQUIRED)
find_package(SOCI CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Poco REQUIRED Data Foundation CppParser Encodings JSON  Crypto Net NetSSL Util Zip ActiveRecord)
find_package(RdKafka CONFIG REQUIRED)
find_package(eclipse-paho-mqtt-c CONFIG REQUIRED)
# find_package(Boost REQUIRED COMPONENTS unit_test_framework)
#target_link_libraries(${_lib_name} PRIVATE Boost::unit_test_framework)
# ${Boost_LIBRARIES}


include(GenerateExportHeader)
include_directories(include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)
include_directories(${PahoMqttCpp_INCLUDE_DIRS})
include_directories(third_party)
include_directories(third_party/concurrentqueue)
include_directories(third_party/mqtt)
include_directories(third_party/kafka)
include_directories(db)
include_directories(db/rd)
include_directories(utils)
include_directories(dds)
include_directories(src/proto)
include_directories(${Protobuf_INCLUDE_DIR})


message(STATUS "Configuring cam-app ...")
file(GLOB DDS_COMM_SOURCES_CXX "dds/*.cxx")
file(GLOB DDS_NODE_SOURCES_CXX "src/*.cxx")
file(GLOB PB_SOURCES_CXX "src/proto/*.cc")
file(GLOB MONITOR_PB_SOURCES_CXX "dds/*.cc")
file(GLOB DDS_NODE_SOURCES_CPP "src/*.cpp")
file(GLOB UTILS_SOURCES_CPP "utils/*.cpp")
file(GLOB CONFIG_SOURCES_CPP "utils/config/*.cpp")
file(GLOB INI_NODE_SOURCES_CPP "utils/ini/*.cpp")
file(GLOB TOPCI_BASE_SOURCES_CPP "dds/*.cpp")
file(GLOB DB_SOURCES_CPP "db/*.cpp")
file(GLOB DB_RD_SOURCES_CPP "db/rd/*.cpp")
file(GLOB MQTT_SOURCES_CPP "third_party/mqtt/*.cpp")
file(GLOB KAFKA_SOURCES_CPP "third_party/kafka/*.cpp")

if(NOT BUILD_SHARED_LIBS)
  set(_target_zip_archives  ${CppMicroServices_LIBRARIES})
  set(_srcs main.cpp)
  add_executable(${_lib_name}  ${DDS_COMM_SOURCES_CXX} ${DDS_COMM_SOURCES_CPP} ${DDS_NODE_SOURCES_CXX} ${DDS_NODE_SOURCES_CPP} ${INI_NODE_SOURCES_CPP} ${CONFIG_SOURCES_CPP} ${TOPCI_BASE_SOURCES_CPP} ${DB_SOURCES_CPP} ${DB_RD_SOURCES_CPP} ${UTILS_SOURCES_CPP} ${PB_SOURCES_CXX} ${MQTT_SOURCES_CPP} ${KAFKA_SOURCES_CPP} ${MONITOR_PB_SOURCES_CXX})

  	IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
		target_link_libraries(${_lib_name} PRIVATE ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES} protobuf::protobuf zenohcxx::zenohc SOCI::soci_core SOCI::soci_empty SOCI::soci_mysql  spdlog::spdlog spdlog::spdlog_header_only   Poco::NetSSL Poco::Net  uuid eclipse-paho-mqtt-c::paho-mqtt3as-static stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m RdKafka::rdkafka RdKafka::rdkafka++ )
	ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
		target_link_libraries(${_lib_name} PRIVATE ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES} protobuf::protobuf zenohcxx::zenohc SOCI::soci_core SOCI::soci_empty SOCI::soci_mysql spdlog::spdlog spdlog::spdlog_header_only  Poco::NetSSL Poco::Net  eclipse-paho-mqtt-c::paho-mqtt3as-static RdKafka::rdkafka RdKafka::rdkafka++ )
	ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
		target_link_libraries(${_lib_name} PRIVATE ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES} protobuf::protobuf zenohcxx::zenohc soci_core soci_empty soci_mysql  spdlog::spdlog spdlog::spdlog_header_only  unofficial::concurrentqueue::concurrentqueue Poco::NetSSL Poco::Net  uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m RdKafka::rdkafka RdKafka::rdkafka++ )
	ELSE ()
		target_link_libraries(${_lib_name} PRIVATE ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES} protobuf::protobuf zenohcxx::zenohc soci_core soci_empty soci_mysql  spdlog::spdlog spdlog::spdlog_header_only  unofficial::concurrentqueue::concurrentqueue Poco::NetSSL Poco::Net uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m RdKafka::rdkafka RdKafka::rdkafka++ )
	ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

  usFunctionAddResources(TARGET ${_lib_name}
                         BUNDLE_NAME main
                         ZIP_ARCHIVES ${_target_zip_archives})
  
  target_compile_definitions(${_lib_name} PRIVATE
		$<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
		$<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
	)

  usFunctionEmbedResources(TARGET ${_lib_name})
  
  #if (WIN32)
    #target_link_options(${_lib_name} PRIVATE /NODEFAULTLIB:LIBCMT)
#endif()
  
  message(STATUS ${BIN_INSTALL_DIR})
  
  install(TARGETS ${_lib_name}  RUNTIME DESTINATION ${BIN_INSTALL_DIR})
else()
  #add_executable(${_lib_name} main.cpp)
  #target_link_libraries(${_lib_name} ${CppMicroServices_LIBRARIES})
  
  #add_executable(${_lib_name} ${DDS_NODE_SOURCES_CXX} ${DDS_NODE_SOURCES_CPP} ${INI_NODE_SOURCES_CPP} ${CONFIG_SOURCES_CPP} ${TOPCI_BASE_SOURCES_CPP}${DB_SOURCES_CPP} ${DB_RD_SOURCES_CPP} ${UTILS_SOURCES_CPP}  ${HWINFO_CPP} ${HWINFO_SUB_CPP})
  add_executable(${_lib_name}  ${DDS_COMM_SOURCES_CXX} ${DDS_COMM_SOURCES_CPP} ${DDS_NODE_SOURCES_CXX} ${DDS_NODE_SOURCES_CPP} ${INI_NODE_SOURCES_CPP} ${CONFIG_SOURCES_CPP} ${TOPCI_BASE_SOURCES_CPP} ${DB_SOURCES_CPP} ${DB_RD_SOURCES_CPP} ${UTILS_SOURCES_CPP}  ${PB_SOURCES_CXX}  ${KAFKA_SOURCES_CPP} ${MONITOR_PB_SOURCES_CXX})

  target_compile_definitions(${_lib_name} PRIVATE
		$<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
		$<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
	)
  #target_link_libraries(Node ${CppMicroServices_LIBRARIES} fastrtps fastcdr foonathan_memory SOCI::soci_core SOCI::soci_empty SOCI::soci_mysql spdlog::spdlog spdlog::spdlog_header_only ${Boost_LIBRARIES} unofficial::concurrentqueue::concurrentqueue)
  	IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
		target_link_libraries(${_lib_name} PRIVATE ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES} protobuf::protobuf zenohcxx::zenohc SOCI::soci_core SOCI::soci_empty SOCI::soci_mysql  spdlog::spdlog spdlog::spdlog_header_only   Poco::NetSSL Poco::Net  uuid eclipse-paho-mqtt-c::paho-mqtt3as-static stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m RdKafka::rdkafka RdKafka::rdkafka++ )
	ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
		target_link_libraries(${_lib_name} PRIVATE ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES} protobuf::protobuf zenohcxx::zenohc SOCI::soci_core SOCI::soci_empty SOCI::soci_mysql spdlog::spdlog spdlog::spdlog_header_only  Poco::NetSSL Poco::Net  eclipse-paho-mqtt-c::paho-mqtt3as-static RdKafka::rdkafka RdKafka::rdkafka++ )
	ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
		target_link_libraries(${_lib_name} PRIVATE ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES} protobuf::protobuf zenohcxx::zenohc soci_core soci_empty soci_mysql  spdlog::spdlog spdlog::spdlog_header_only  unofficial::concurrentqueue::concurrentqueue Poco::NetSSL Poco::Net  uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m RdKafka::rdkafka RdKafka::rdkafka++ )
	ELSE ()
		target_link_libraries(${_lib_name} PRIVATE ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES} protobuf::protobuf zenohcxx::zenohc soci_core soci_empty soci_mysql  spdlog::spdlog spdlog::spdlog_header_only  unofficial::concurrentqueue::concurrentqueue Poco::NetSSL Poco::Net  uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m RdKafka::rdkafka RdKafka::rdkafka++ )
	ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

	#if (WIN32)
    #target_link_options(${_lib_name} PRIVATE /NODEFAULTLIB:LIBCMT)
#endif()
endif()


###




