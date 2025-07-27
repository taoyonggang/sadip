set (_lib_name monitor-app)

cmake_minimum_required(VERSION 3.16)
project(${_lib_name} VERSION 1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 CMake policy
cmake_policy(SET CMP0091 NEW)

# 设置 debug 后缀
set(CMAKE_DEBUG_POSTFIX "d")

add_definitions(-D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)


include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)


# 起始

find_package(zenohc) 
find_package(zenohcxx)
find_package(spdlog CONFIG REQUIRED)
find_package(protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)


include_directories(include)


include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(../../db)
include_directories(../../third_party)
include_directories(../../third_party/concurrentqueue)
include_directories(../../db/rd)
include_directories(../../utils)
include_directories(../../dds)


message(STATUS "Configuring DDS monitor...")
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
file(GLOB MONITOR_PROTO_CPP "../../dds/*.cc")


if(NOT BUILD_SHARED_LIBS)
  set(_srcs main.cpp)
  add_executable(${_lib_name} ${DDS_COMM_SOURCES_CXX} ${DDS_COMM_SOURCES_CPP} ${DDS_NODE_SOURCES_CXX} ${DDS_NODE_SOURCES_CPP} ${INI_NODE_SOURCES_CPP} ${CONFIG_SOURCES_CPP} ${TOPCI_BASE_SOURCES_CPP} ${DB_SOURCES_CPP} ${DB_RD_SOURCES_CPP} ${UTILS_SOURCES_CPP} ${MONITOR_PROTO_CPP})
 
 
 # OpenCL::OpenCL OpenCL::HeadersCpp OpenCL::Headers   Poco::Foundation Poco::Util Poco::NetSSL Poco::Net Poco::Zip 
  	IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
		target_link_libraries(${_lib_name}  protobuf::protobuf zenohcxx::zenohc SOCI::soci_core SOCI::soci_empty SOCI::soci_mysql  spdlog::spdlog   uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m)
	ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
		target_link_libraries(${_lib_name}  protobuf::protobuf zenohcxx::zenohc SOCI::soci_core  SOCI::soci_mysql spdlog::spdlog  )
	ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
		target_link_libraries(${_lib_name}  protobuf::protobuf zenohcxx::zenohc SOCI::soci_core SOCI::soci_empty SOCI::soci_mysql  spdlog::spdlog  unofficial::concurrentqueue::concurrentqueue uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m)
	ELSE ()
		target_link_libraries(${_lib_name}  protobuf::protobuf zenohcxx::zenohc SOCI::soci_core SOCI::soci_empty SOCI::soci_mysql  spdlog::spdlog  unofficial::concurrentqueue::concurrentqueue  uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m)
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
  add_executable(${_lib_name}  ${DDS_COMM_SOURCES_CXX} ${DDS_COMM_SOURCES_CPP} ${DDS_NODE_SOURCES_CXX} ${DDS_NODE_SOURCES_CPP} ${INI_NODE_SOURCES_CPP} ${CONFIG_SOURCES_CPP} ${TOPCI_BASE_SOURCES_CPP}${DB_SOURCES_CPP} ${DB_RD_SOURCES_CPP} ${UTILS_SOURCES_CPP}  ${MONITOR_PROTO_CPP})
  target_compile_definitions(${_lib_name} PRIVATE
		$<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
		$<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
	)
	# Poco::NetSSL Poco::Net Poco::Zip  
    IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
		target_link_libraries(${_lib_name}   protobuf::protobuf soci_core soci_empty soci_mysql  spdlog::spdlog spdlog::spdlog_header_only  unofficial::concurrentqueue::concurrentqueue  uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m)
	ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
		target_link_libraries(${_lib_name}   protobuf::protobuf SOCI::soci_core SOCI::soci_empty SOCI::soci_mysql spdlog::spdlog spdlog::spdlog_header_only  )
	ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
		target_link_libraries(${_lib_name} }   protobuf::protobuf soci_core soci_empty soci_mysql  spdlog::spdlog spdlog::spdlog_header_only  unofficial::concurrentqueue::concurrentqueue  uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m)
	ELSE ()
		target_link_libraries(${_lib_name}   protobuf::protobuf soci_core soci_empty soci_mysql  spdlog::spdlog spdlog::spdlog_header_only  unofficial::concurrentqueue::concurrentqueue  uuid stdc++fs stdc++ mysqlclient zstd ssl crypto resolv m)
	ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

  install(TARGETS ${_lib_name}  RUNTIME DESTINATION ${BIN_INSTALL_DIR})

endif()


#结束
