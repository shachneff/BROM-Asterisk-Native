#ifndef __STDAFX_H__
#define __STDAFX_H__

#ifdef _WINDOWS
#include <Winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <Windows.h>
#endif //_WINDOWS

#if defined(__linux__) || defined(__APPLE__)
#define LINUX_OR_MACOS
#endif

#endif //__STDAFX_H__