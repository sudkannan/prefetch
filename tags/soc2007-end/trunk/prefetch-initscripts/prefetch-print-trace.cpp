#include "prefetch_lib.h"
#include "prefetch_types.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	prefetch_trace_record_t record;
	int r;
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
