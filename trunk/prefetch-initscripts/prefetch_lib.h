#ifndef __PREFETCH_LIB_H
#define __PREFETCH_LIB_H
/**
 * This file contains declarations of low-level library used to access prefetch functionality.
*/

#include "prefetch_types.h"

typedef int prefetch_trace_desc_t;

int open_trace_file(char *filename, prefetch_trace_desc_t *file);
int read_prefetch_record(prefetch_trace_desc_t file, prefetch_trace_record_t *record);
int close_trace_file(prefetch_trace_desc_t file);

void fill_trace_file_header(prefetch_trace_header_t *header);

#endif //__PREFETCH_LIB_H
