/*
    File: OSAL Log

    Compile-time settings for the OSAL Log: 6 definitions.
*/

/*
    Macros: OSAL Log macros

    PRINT    - Expands to printf(the arguments); printf("\r\n").
    CRITICAL - Expands to LogAlways( (the arguments) ).
    ERROR    - Expands to LogError( (the arguments) ).
    WARN     - Expands to LogWarn( (the arguments) ).
    INFO     - Expands to LogInfo( (the arguments) ).
    DEBUG    - Expands to LogDebug( (the arguments) ).
*/

#ifndef __OSAL_LOG__
#define __OSAL_LOG__
#include <stdarg.h>
#include <stdio.h>
#include "osal_log_config.h"
#include <logging_stack.h>

/*The defines are defined in decreasing order of level*/

#define PRINT(...) printf(__VA_ARGS__); printf("\r\n")
#define CRITICAL(...) LogAlways( (__VA_ARGS__) )
#define ERROR(...) LogError( (__VA_ARGS__) )
#define WARN(...) LogWarn( (__VA_ARGS__) )
#define INFO(...) LogInfo( (__VA_ARGS__) )
#define DEBUG(...) LogDebug( (__VA_ARGS__) )

#endif // __OSAL_LOG__
