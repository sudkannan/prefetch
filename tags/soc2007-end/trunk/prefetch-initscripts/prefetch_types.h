#ifndef __PREFETCH_TYPES_H
#define __PREFETCH_TYPES_H
/**
  * This file contains types and constants shared with kernel implementation.
  * Please keep in sync.
*/

#define MINORBITS	20
#define MINORMASK	((1U << MINORBITS) - 1)

#define MAJOR(dev)	((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev)	((unsigned int) ((dev) & MINORMASK))

typedef unsigned int kdev_t;
typedef unsigned long pgoff_t;
typedef unsigned short u16;

typedef struct {
	kdev_t device;
	unsigned long inode_no;
	pgoff_t range_start;
	pgoff_t range_length;
} prefetch_trace_record_t;


extern char trace_file_magic[4];

enum {
	PREFETCH_FORMAT_VERSION_MAJOR = 1,
	PREFETCH_FORMAT_VERSION_MINOR = 0
};

/**
 * Trace on-disk header.
 * Major version is increased with major changes of format.
 * If you do not support this format explicitely, do not read other fields.
 * Minor version is increased with backward compatible changes and
 * you can read other fields and raw data, provided that you read
 * trace data from @data_start offset in file.
*/
typedef struct {
	///Trace file signature - should contain trace_file_magic
	char magic[4]; 
	///Major version of trace file format
	u16 version_major;
	///Minor version of trace file format
	u16 version_minor;
	///Trace raw data start
	u16 data_start;
} prefetch_trace_header_t;

#endif //__PREFETCH_TYPES_H
