#include "prefetch_types.h"
#include "prefetch_lib.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

/**
 * This file contains definition of low-level library used to access prefetch functionality.
*/

char trace_file_magic[4] = {'P', 'F', 'C', 'H' };

/**
	Opens trace file from file named @filename. Puts file descriptor in @file in case of success.
	File is seeked to start of trace data, so you can start reading immediately.
	Returns 0 if success, <0 in case of error.
*/
int open_trace_file(char *filename, prefetch_trace_desc_t *file)
{
	prefetch_trace_header_t header;
	int r;
	
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("Cannot open trace file");
		return -1;
	}
	
	r = read(fd, &header, sizeof(header));
	if (r == -1) {
		perror("Cannot read trace file header");
		close(fd);
		return -1;
	}
	
	if (strncmp(&header.magic[0], &trace_file_magic[0], sizeof(header.magic)) != 0) {
		fprintf(stderr, "Invalid trace file signature\n");
		close(fd);
		return -1;
	}
	
	if (header.version_major != PREFETCH_FORMAT_VERSION_MAJOR) {
		fprintf(stderr, "Incompatible trace file format version %d\n", header.version_major);
		close(fd);
		return -1;
	}
	
	if (header.data_start < sizeof(header)) {
		fprintf(stderr, "Invalid trace file header, data start=%d\n", header.data_start);
		close(fd);
		return -1;
	}
	
	if (lseek(fd, header.data_start, SEEK_SET) == -1) {
		fprintf(stderr, "Cannot seek to file start, data start=%d\n", header.data_start);
		close(fd);
		return -1;
	}
	
	*file = fd;
	return 0;
}

/**
	Fills in header used of trace file, it can be then written to file, with data coming after it.
*/
void fill_trace_file_header(prefetch_trace_header_t *header)
{
	strncpy(&header->magic[0], &trace_file_magic[0], sizeof(header->magic));
	header->version_major = PREFETCH_FORMAT_VERSION_MAJOR;
	header->version_minor = PREFETCH_FORMAT_VERSION_MINOR;
	header->data_start = sizeof(prefetch_trace_header_t);
}

/**
	Closes trace file @file.
	Returns 0 if success, <0 in case of error.
*/
int close_trace_file(prefetch_trace_desc_t file)
{
	if (close(file) == -1) {
		perror("Error closing trace file");
		return -1;
	}
	return 0;
}

/**
	Reads one prefetch record from @file and puts it in @record.
	Returns 0 if successfully read, 1 if end of file, <0 in case of error.
*/
int read_prefetch_record(prefetch_trace_desc_t file, prefetch_trace_record_t *record)
{
	int r;
	int record_size = sizeof(*record);
	//FIXME: use buffered IO
	
	r = read(file, record, record_size);
	if (r == 0) {
		return 1;
	}
	if (r < 0) {
		perror("Error while reading trace file");
		return -1;
	}
	if (r != record_size) {
		//FIXME: read the rest in loop
		fprintf(stderr, "Size read not matching requested size\n");
		return -1;
	}
	return 0;
}
