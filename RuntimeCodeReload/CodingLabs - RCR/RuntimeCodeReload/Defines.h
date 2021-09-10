#include <windows.h>
#include <sstream>
#include <iostream>

#ifdef RCR_ENABLED
#define RCR_ASSERT(condition, message) \
    do { \
    if (! (condition)) { \
    std::stringstream ss(""); \
    ss << "Assertion `" #condition "` failed in " << __FILE__ \
    << " line " << __LINE__ << ": " << std::endl << std::endl << message << std::endl; \
    std::cerr << ss.str(); \
    MessageBoxA(0, ss.str().c_str(), "Runtime Code Reload - ASSERT", MB_ICONERROR); \
    std::exit(EXIT_FAILURE); \
    } \
    } while (false)
#else
#define RCR_ASSERT(condition, message) do { } while (false)
#endif