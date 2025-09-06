#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
	#ifdef EMERAUDE_BUILD_DLL
		#define EMERAUDE_API __declspec(dllexport)
	#else
		#define EMERAUDE_API __declspec(dllimport)
	#endif
#else
	#define EMERAUDE_API __attribute__((visibility("default")))
#endif
