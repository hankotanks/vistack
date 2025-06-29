#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <assert.h>

#define LOG_INFO 0
#define LOG_ERROR 1

#ifdef LOGGING
//
#define LOG(lvl, ...) {\
    FILE* stream = ((lvl) == LOG_ERROR) ? stderr : stdout;\
    fprintf(stream, "%s [%s:%d]: ", ((lvl) == LOG_ERROR) ? "ERROR" : "INFO", __FILE__, __LINE__);\
    fprintf(stream, __VA_ARGS__);\
    fprintf(stream, "\n");\
}
#define LOG_AND_CLOSE(stream, ...) {\
    LOG(LOG_ERROR, __VA_ARGS__);\
    fclose((stream));\
    return NULL;\
}
//
#define ASSERT(cond) for(int _cond = (int) (cond); !(_cond); assert(_cond), _cond = _cond)
#define ASSERT_LOG(cond, ...) ASSERT(cond) LOG(LOG_ERROR, __VA_ARGS__);
//
#else
//
#define LOG(lvl, fmt, ...)
#define LOG_AND_CLOSE(stream, fmt, ...)
//
#define ASSERT(cond)
#define ASSERT_LOG(cond, fmt, ...)
//
#endif /* LOGGING */

#endif /* __LOG_H__ */
