#include "prefetch_trace.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

typedef int prefetch_trace_desc_t;

//FIXME: move these functions to library

/**
	Opens trace file from file named @filename. Puts file descriptor in @file in case of success.
	Returns 0 if success, <0 in case of error.
*/
int open_trace_file(char *filename, prefetch_trace_desc_t *file)
{
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("Cannot open trace file");
		return -1;
	}
	*file = fd;
	return 0;
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
		fprintf(stderr, "Size read not matching requested size");
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	prefetch_trace_record_t record;
	int r;
	int fd;
	char *filename;
	prefetch_trace_desc_t tracefile;
	
	if (argc != 2) {
		printf("Usage: prefetch-print-trace tracefile\n");
		exit(1);
	}
	filename = argv[1];

	r = open_trace_file(filename, &tracefile);
	if (r < 0) {
		fprintf(stderr, "Cannot open trace file %s\n", filename);
		exit(1);
	}

	for (;;) {
		r = read_prefetch_record(tracefile, &record);
		if (r == 1)
			break; //end of file
		if (r < 0) {
			fprintf(stderr, "Cannot read trace file %s\n", filename);
			exit(2);
		}
		printf("%u:%u\t%lu\t%lu\t%lu\n",
			MAJOR(record.device), MINOR(record.device),
			record.inode_no,
			record.range_start, record.range_length
			);
	}
	close_trace_file(tracefile);
	return 0;
}
