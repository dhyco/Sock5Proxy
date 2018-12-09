#pragma once
namespace Base {
	namespace OverPlatform {
#ifdef _WIN32
		//define something for Windows (32-bit and 64-bit, this part is common)
#include "windows.h"
		inline unsigned int GetCpuNum() {
			SYSTEM_INFO sysInfo;
			GetSystemInfo(&sysInfo);
			return sysInfo.dwNumberOfProcessors;
		}
#ifdef _WIN64
		//define something for Windows (64-bit only)
#endif
#elif __APPLE__
#include "TargetConditionals.h"
#if TARGET_IPHONE_SIMULATOR
		// iOS Simulator
#elif TARGET_OS_IPHONE
		// iOS device
#elif TARGET_OS_MAC
		// Other kinds of Mac OS
#else
#   error "Unknown Apple platform"
#endif
#elif __linux__
		// linux
#include "unistd.h"
		static unsigned int GetCpuNum() {
			return sysconf(_SC_NPROCESSORS_ONLN);
		}
#elif __unix__ // all unices not caught above
		// Unix
#elif defined(_POSIX_VERSION)
		// POSIX
#else
#   error "Unknown compiler"
#endif
	}
}