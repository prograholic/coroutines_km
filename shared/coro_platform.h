#pragma once

#if defined (_KERNEL_MODE)

#include <km/platform.h>

#elif defined(CORO_EFI_MODE)

#include <efi/platform.h>

#elif defined(WIN32) || defined(_WIN32)

#include <um/platform.h>

#else

#error Unsupported platform

#endif

#if !defined(CORO_NO_EXCEPTIONS)
#define CORO_HAS_EXCEPTIONS
#endif /* CORO_NO_EXCEPTIONS */

