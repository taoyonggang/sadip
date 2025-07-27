// pch.h
#pragma once

// 确保 std::byte 被禁用
#ifdef _HAS_STD_BYTE
#undef _HAS_STD_BYTE
#endif
#define _HAS_STD_BYTE 0

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

// Windows 头文件的包含顺序
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

// RPC 相关头文件需要特殊处理
//#ifdef __cplusplus
//extern "C" {
//#endif
//#include <rpcndr.h>
//#include <rpc.h>
//#include <rpcnsip.h>
//#ifdef __cplusplus
//}
//#endif
#endif

// STL 头文件
#include <string>
#include <vector>
#include <memory>
#include <iostream>