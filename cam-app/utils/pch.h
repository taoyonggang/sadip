// pch.h
#pragma once

// ȷ�� std::byte ������
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

// Windows ͷ�ļ��İ���˳��
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

// RPC ���ͷ�ļ���Ҫ���⴦��
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

// STL ͷ�ļ�
#include <string>
#include <vector>
#include <memory>
#include <iostream>