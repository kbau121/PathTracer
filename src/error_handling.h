#pragma once

#include <cstdio>

#include <utils.h>

inline bool _isError(bool result)
{
    return !result;
}

inline void _logError(bool result)
{
    UNUSED(result);

    printf("Error: false\n");
}

template<typename T>
inline bool _logIfErrorWrapper(
    T result,
    const char* exprString,
    const char* filePath,
    int lineNumber)
{
    if (_isError(result)) {
        _logError(result);
        printf("    %s\n", exprString);
        printf("    at %s:%d\n", filePath, lineNumber);
        return false;
    }

    return true;
}

#define IS_ERROR(EXPR) _isError((EXPR))
#define IGNORE_ERROR(EXPR) \
    do {                   \
        (EXPR);            \
    } while (false)
#define RETURN_IF_ERROR(EXPR)   \
    do {                        \
        if (_isError((EXPR))) { \
            return false;       \
        }                       \
    } while (false)
#define LOG_IF_ERROR(EXPR) _logIfErrorWrapper((EXPR), #EXPR, __FILE__, __LINE__)
#define LOG_AND_RETURN_IF_ERROR(EXPR) \
    do {                              \
        if (!LOG_IF_ERROR(EXPR)) {    \
            return false;             \
        }                             \
    } while (false)
