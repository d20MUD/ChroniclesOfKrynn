#ifndef KRYNN_DURABLE_FILE_H
#define KRYNN_DURABLE_FILE_H
#include <stdbool.h>
#include <stddef.h>
/* Atomic replacement, including file and parent-directory fsync. A false result
 * after rename is ambiguous: callers must retain their receipt and retry. */
bool durable_file_replace(const char *path, const void *data, size_t size);
#ifdef LEGENDARY_TRANSACTION_TEST
extern int durable_file_fault;
#endif
#endif
