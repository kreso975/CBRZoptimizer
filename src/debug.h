#ifndef DEBUG_LOGGING_H
#define DEBUG_LOGGING_H

#if defined(DEBUG) || defined(_DEBUG)

#define DEBUG_PRINTF(fmt, ...)                  \
  do                                            \
  {                                             \
    wchar_t dbg_buf[512];                       \
    swprintf_s(dbg_buf, 512, fmt, __VA_ARGS__); \
    OutputDebugStringW(dbg_buf);                \
  } while (0)

#define DEBUG_PRINT(msg)                        \
  do                                            \
  {                                             \
    OutputDebugStringW(msg);                    \
  } while (0)

#else

#define DEBUG_PRINTF(fmt, ...) \
  do                           \
  {                            \
  } while (0)

#define DEBUG_PRINT(msg)       \
  do                           \
  {                            \
  } while (0)

#endif

#endif