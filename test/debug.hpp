#pragma once

#include <iostream>

// Set DEBUG to 0 to compile out the diagnostic output.
#ifndef DEBUG
#define DEBUG 1
#endif

#if DEBUG
#define DEBUG_LOG(tag, message) \
	do { std::cerr << "[DEBUG][" << tag << "] " << message << std::endl; } while (false)
#define DEBUG_WARN(tag, message) \
	do { std::cerr << "[WARN][" << tag << "] " << message << std::endl; } while (false)
#define DEBUG_ERROR(tag, message) \
	do { std::cerr << "[ERROR][" << tag << "] " << message << std::endl; } while (false)
#else
#define DEBUG_LOG(tag, message) do { } while (false)
#define DEBUG_WARN(tag, message) do { } while (false)
#define DEBUG_ERROR(tag, message) do { } while (false)
#endif
