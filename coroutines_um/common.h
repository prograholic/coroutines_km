#pragma once

#include <vector>
#include <string>
#include <iostream>

#include <Windows.h>

typedef std::vector<unsigned char> blob_t;

#define STR(x) STR2(#x)
#define STR2(x) #x

#define TRACE() std::cerr << __FUNCTION__ << ": "

constexpr size_t DataSize = 16 * 1024 * 1024;



void TryWinApiWaiter();
void TryCoroWaiter();
void TestChronoAwait();
void ConnectToDriver();
