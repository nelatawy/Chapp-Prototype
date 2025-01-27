#pragma once
#define _WIN32_WINNT 0x0A00 // Target Windows 10/11
#define DEFAULT_SOCKET 8040


// Windows header files
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

//IO headers
#include <stdio.h>
#include <iostream>

// Standard C++ headers
#include <string>
#include <vector>

// Ensure you link against Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
