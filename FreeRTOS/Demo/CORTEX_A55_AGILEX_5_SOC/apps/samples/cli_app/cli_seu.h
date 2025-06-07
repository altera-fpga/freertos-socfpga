#ifndef __CLI_SEU__
#define __CLI_SEU__

BaseType_t inject_seu_err( char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString );
BaseType_t read_seu_err( char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString );
BaseType_t ecc_injection( char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString );
BaseType_t seu_err_stats( char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString );
#endif
