cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 CMake policy
cmake_policy(SET CMP0091 NEW)

# 设置 debug 后缀
set(CMAKE_DEBUG_POSTFIX "d")

add_definitions(-D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)


include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)

# 设置变量
set(_lib_name node)

# 项目定义
project(${_lib_name} VERSION 1 LANGUAGES CXX)


find_package(nlohmann_json CONFIG REQUIRED)

find_package(zenohc) 
find_package(zenohcxx)
    

#set(Boost_USE_STATIC_LIBS ON)
set(BOOST_COMPONENTS filesystem iostreams system thread regex chrono atomic )
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

#if(NOT TARGET spdlog)
find_package(spdlog CONFIG REQUIRED)
#endif()

#find_package(unofficial-concurrentqueue CONFIG REQUIRED)

# 查找所有需要的包
find_package(RdKafka REQUIRED)
find_package(protobuf REQUIRED)
# 查找 Poco 包
find_package(Poco REQUIRED COMPONENTS Foundation Util Net NetSSL Crypto JSON XML)
# 添加调试信息
message(STATUS "Poco_INCLUDE_DIRS: ${Poco_INCLUDE_DIRS}")
message(STATUS "Poco_LIBRARIES: ${Poco_LIBRARIES}")

find_package(OpenSSL REQUIRED)
find_package(AWSSDK REQUIRED)
find_package(Boost REQUIRED)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(Protobuf REQUIRED)

# 打印更详细的库信息
foreach(comp Data Foundation Encodings XML JSON Crypto Net NetSSL Util Zip ActiveRecord)
    get_target_property(POCO_${comp}_IMPLIB Poco::${comp} IMPORTED_IMPLIB_RELEASE)
    get_target_property(POCO_${comp}_DLL Poco::${comp} IMPORTED_LOCATION_RELEASE)
    get_target_property(POCO_${comp}_INCLUDE Poco::${comp} INTERFACE_INCLUDE_DIRECTORIES)
    
    message(STATUS "Poco::${comp}:")
    message(STATUS "  IMPLIB: ${POCO_${comp}_IMPLIB}")
    message(STATUS "  DLL: ${POCO_${comp}_DLL}")
    message(STATUS "  INCLUDE: ${POCO_${comp}_INCLUDE}")
endforeach()

# 查找 Poco 包的安装路径
if(DEFINED ENV{CONAN_POCO_ROOT})
    message(STATUS "Poco Conan Root: $ENV{CONAN_POCO_ROOT}")
endif()

# 打印 CMake 变量
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message(STATUS "Poco_DIR: ${Poco_DIR}")


find_package(AWSSDK REQUIRED COMPONENTS s3)
#find_package(aws-sdk-cpp REQUIRED)
message(STATUS "AWSSDK_FOUND: ${AWSSDK_FOUND}")
message(STATUS "AWSSDK_INCLUDE_DIRS: ${AWSSDK_INCLUDE_DIRS}")
message(STATUS "AWSSDK_LIBRARIES: ${AWSSDK_LIBRARIES}")
message(STATUS "AWSSDK_LIB_DIR: ${AWSSDK_LIB_DIR}")



include_directories(include)

include_directories(${AWSSDK_INCLUDE_DIRS})


include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(../../db)
include_directories(../../third_party)
include_directories(../../third_party/concurrentqueue)
include_directories(../../db/rd)
include_directories(../../utils)
include_directories(../../dds)
include_directories(proto)

message(STATUS "Configuring Zenoh Node...")
file(GLOB DDS_COMM_SOURCES_CXX "../../dds/*.cxx")
file(GLOB DDS_COMM_SOURCES_CPP "../../dds/*.cpp")
file(GLOB DDS_NODE_SOURCES_CXX "*.cxx")
file(GLOB DDS_NODE_SOURCES_CPP "*.cpp")
file(GLOB UTILS_SOURCES_CPP "../../utils/*.cpp")
file(GLOB CONFIG_SOURCES_CPP "../../utils/config/*.cpp")
file(GLOB INI_NODE_SOURCES_CPP "../../utils/ini/*.cpp")
file(GLOB TOPCI_BASE_SOURCES_CPP "../../dds/*.cpp")
file(GLOB DB_SOURCES_CPP "../../db/*.cpp")
file(GLOB DB_RD_SOURCES_CPP "../../db/rd/*.cpp")
file(GLOB HWINFO_CPP "hwinfo/*.cpp")
file(GLOB HWINFO_SUB_CPP "hwinfo/*/*.cpp")
file(GLOB HWINFO_SUB_SUB_CPP "hwinfo/*/*/*.cpp")

file(GLOB AWS_S3_CPP "../../utils/aws/s3/*.cpp")
file(GLOB PROTO_CPP "proto/*.cc")
file(GLOB MONITOR_PROTO_CPP "../../dds/*.cc")

# OpenCL 配置
find_package(OpenCLHeaders REQUIRED)
find_package(OpenCLHeadersCpp REQUIRED)
find_package(OpenCLICDLoader REQUIRED)



if(NOT BUILD_SHARED_LIBS)
  set(_target_zip_archives  ${CppMicroServices_LIBRARIES})
  set(_srcs main.cpp)
  add_executable(${_lib_name} ${DDS_COMM_SOURCES_CXX} ${DDS_COMM_SOURCES_CPP} ${DDS_NODE_SOURCES_CXX} ${DDS_NODE_SOURCES_CPP} ${INI_NODE_SOURCES_CPP} ${CONFIG_SOURCES_CPP} ${TOPCI_BASE_SOURCES_CPP} ${DB_SOURCES_CPP} ${DB_RD_SOURCES_CPP} ${UTILS_SOURCES_CPP} ${HWINFO_CPP} ${HWINFO_SUB_CPP} ${HWINFO_SUB_SUB_CPP} ${AWSSDK_INCLUDE_DIRS} ${AWS_S3_CPP} ${PROTO_CPP} ${MONITOR_PROTO_CPP})

   #add_executable(${_lib_name} ${DDS_NODE_SOURCES_CXX} ${DDS_NODE_SOURCES_CPP} ${INI_NODE_SOURCES_CPP} ${CONFIG_SOURCES_CPP} ${TOPCI_BASE_SOURCES_CPP} ${DB_SOURCES_CPP} ${DB_RD_SOURCES_CPP} ${UTILS_SOURCES_CPP} ${HWINFO_CPP} ${HWINFO_SUB_CPP} ${HWINFO_SUB_SUB_CPP}) 
  	IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
		target_link_libraries(${_lib_name}  ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES}  ${Protobuf_LIBRARIES} OpenCL::OpenCL OpenCL::HeadersCpp OpenCL::Headers protobuf::protobuf zenohcxx::zenohc SOCI::soci_core SOCI::soci_empty SOCI::soci_mysql  spdlog::spdlog Poco::Foundation Poco::Util Poco::NetSSL Poco::Net Poco::Zip AWS::aws-sdk-cpp-core AWS::aws-sdk-cpp-s3   uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m)
	ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
		target_link_libraries(${_lib_name}  ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES}  ${Protobuf_LIBRARIES} OpenCL::OpenCL OpenCL::HeadersCpp OpenCL::Headers protobuf::protobuf zenohcxx::zenohc SOCI::soci_core  SOCI::soci_mysql spdlog::spdlog  Poco::Foundation Poco::Util Poco::NetSSL Poco::Net Poco::Zip  AWS::aws-sdk-cpp-core AWS::aws-sdk-cpp-s3  )
	ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
		target_link_libraries(${_lib_name}  ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES}  ${Protobuf_LIBRARIES} OpenCL::OpenCL OpenCL::HeadersCpp OpenCL::Headers protobuf::protobuf zenohcxx::zenohc SOCI::soci_core SOCI::soci_empty SOCI::soci_mysql  spdlog::spdlog  unofficial::concurrentqueue::concurrentqueue Poco::Foundation Poco::Util  Poco::NetSSL Poco::Net Poco::Zip  AWS::aws-sdk-cpp-core AWS::aws-sdk-cpp-s3  uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m)
	ELSE ()
		target_link_libraries(${_lib_name}  ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES}  ${Protobuf_LIBRARIES} OpenCL::OpenCL OpenCL::HeadersCpp OpenCL::Headers protobuf::protobuf zenohcxx::zenohc SOCI::soci_core SOCI::soci_empty SOCI::soci_mysql  spdlog::spdlog  unofficial::concurrentqueue::concurrentqueue Poco::NetSSL Poco::Foundation Poco::Util  Poco::Net Poco::Zip AWS::aws-sdk-cpp-core AWS::aws-sdk-cpp-s3  uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m)
	ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

  usFunctionAddResources(TARGET ${_lib_name}
                         BUNDLE_NAME main
                         ZIP_ARCHIVES ${_target_zip_archives})
  
  target_compile_definitions(${_lib_name} PRIVATE
		$<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
		$<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
	)

  usFunctionEmbedResources(TARGET ${_lib_name})
  
  message(STATUS ${BIN_INSTALL_DIR})
  
  install(TARGETS ${_lib_name}  RUNTIME DESTINATION ${BIN_INSTALL_DIR})
else()
  #add_executable(${_lib_name} main.cpp)
  #target_link_libraries(${_lib_name} ${CppMicroServices_LIBRARIES})
  
  add_executable(${_lib_name}  ${DDS_COMM_SOURCES_CXX} ${DDS_COMM_SOURCES_CPP} ${DDS_NODE_SOURCES_CXX} ${DDS_NODE_SOURCES_CPP} ${INI_NODE_SOURCES_CPP} ${CONFIG_SOURCES_CPP} ${TOPCI_BASE_SOURCES_CPP}${DB_SOURCES_CPP} ${DB_RD_SOURCES_CPP} ${UTILS_SOURCES_CPP}  ${HWINFO_CPP} ${HWINFO_SUB_CPP}  ${HWINFO_SUB_SUB_CPP} ${AWSSDK_INCLUDE_DIRS}  ${AWS_S3_CPP} ${PROTO_CPP} ${MONITOR_PROTO_CPP})
  target_compile_definitions(${_lib_name} PRIVATE
		$<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
		$<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
	)
  #target_link_libraries(${_lib_name} ${CppMicroServices_LIBRARIES} fastrtps fastcdr foonathan_memory SOCI::soci_core SOCI::soci_empty SOCI::soci_mysql spdlog::spdlog spdlog::spdlog_header_only ${Boost_LIBRARIES} unofficial::concurrentqueue::concurrentqueue)
    IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
		target_link_libraries(${_lib_name}  ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES}  ${Protobuf_LIBRARIES} soci_core soci_empty soci_mysql  spdlog::spdlog spdlog::spdlog_header_only  unofficial::concurrentqueue::concurrentqueue Poco::NetSSL Poco::Net Poco::Zip AWS::aws-sdk-cpp-core AWS::aws-sdk-cpp-s3   uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m)
	ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
		target_link_libraries(${_lib_name}  ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES}  ${Protobuf_LIBRARIES} SOCI::soci_core SOCI::soci_empty SOCI::soci_mysql spdlog::spdlog spdlog::spdlog_header_only  Poco::NetSSL Poco::Net Poco::Zip AWS::aws-sdk-cpp-core AWS::aws-sdk-cpp-s3  )
	ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
		target_link_libraries(${_lib_name}  ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES}  ${Protobuf_LIBRARIES} soci_core soci_empty soci_mysql  spdlog::spdlog spdlog::spdlog_header_only  unofficial::concurrentqueue::concurrentqueue Poco::NetSSL Poco::Net Poco::Zip AWS::aws-sdk-cpp-core AWS::aws-sdk-cpp-s3   uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m)
	ELSE ()
		target_link_libraries(${_lib_name}  ${CppMicroServices_LIBRARIES} ${Boost_LIBRARIES}  ${Protobuf_LIBRARIES} soci_core soci_empty soci_mysql  spdlog::spdlog spdlog::spdlog_header_only  unofficial::concurrentqueue::concurrentqueue Poco::NetSSL Poco::Net Poco::Zip AWS::aws-sdk-cpp-core AWS::aws-sdk-cpp-s3   uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m)
	ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

  install(TARGETS ${_lib_name}  RUNTIME DESTINATION ${BIN_INSTALL_DIR})

endif()
