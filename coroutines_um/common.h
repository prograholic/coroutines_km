#pragma once

#include <vector>
#include <string>
#include <iostream>

#include <Windows.h>

typedef std::vector<unsigned char> blob_t;

#define STR(x) STR2(#x)
#define STR2(x) #x

typedef std::pair<std::string, DWORD> error_info_t;

#define EXPECT_EX(status, error) \
do { if (!(status)) {auto le = (error); throw error_info_t(std::make_pair(STR(status), le));}} while(0)

#define TRACE() std::cerr << __FUNCTION__ << ": "

#define CALL_SPY() TRACE() << __LINE__ << std::endl


#define EXPECT(status) EXPECT_EX(status, ::GetLastError())

constexpr size_t DataSize = 16 * 1024 * 1024;


void TryWinApiWaiter();
void TryCoroWaiter();
void TryCoroWaiterNoexcept();
void TestChronoAwait();
