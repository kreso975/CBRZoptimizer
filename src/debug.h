#ifndef DEBUG_LOGGING_H
#define DEBUG_LOGGING_H

#if defined(DEBUG) || defined(_DEBUG)
  #define DEBUG_PRINT(msg) OutputDebugStringW(msg)
#else
  #define DEBUG_PRINT(msg) do {} while (0)
#endif

#endif