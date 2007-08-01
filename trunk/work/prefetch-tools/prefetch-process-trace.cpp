#include "prefetch_trace.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <vector>
#include <functional>
#include <algorithm>

using namespace std;

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

typedef struct {
	prefetch_trace_record_t record;
	int index;
} record_data_t;

typedef vector<record_data_t> records_container_t;

typedef struct {
	records_container_t records;
} trace_file_data_t;

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

/**
	Reads conents of trace file @filename to @data structure.
	Returns 0 if successfully read, <0 in case of error.
*/
int trace_file_read_data(char *filename, trace_file_data_t *data)
{
	static int index = 0;
	prefetch_trace_desc_t tracefile;
	record_data_t rdata;
	int r;
	
	r = open_trace_file(filename, &tracefile);
	if (r < 0) {
		fprintf(stderr, "Cannot open trace file %s\n", filename);
		return -1;
	}

	for (;;) {
		r = read_prefetch_record(tracefile, &rdata.record);
		if (r == 1)
			break; //end of file
		if (r < 0) {
			fprintf(stderr, "Cannot read trace file %s\n", filename);
			return -1;
		}
		rdata.index = index;
		++index;
		
		data->records.push_back(rdata);
	}
	close_trace_file(tracefile);
	return 0;
}

int trace_cmp(const void *p1, const void *p2)
{
	prefetch_trace_record_t *r1 = (prefetch_trace_record_t*)p1;
	prefetch_trace_record_t *r2 = (prefetch_trace_record_t*)p2;

	if (r1->device < r2->device)
		return -1;
	if (r1->device > r2->device)
		return 1;

	if (r1->inode_no < r2->inode_no)
		return -1;
	if (r1->inode_no > r2->inode_no)
		return 1;
	
	if (r1->range_start < r2->range_start)
		return -1;
	if (r1->range_start > r2->range_start)
		return 1;
	
	//longer range_length is preferred as we want to fetch large fragments first
	if (r1->range_length < r2->range_length)
		return 1;
	if (r1->range_length > r2->range_length)
		return -1;
	return 0;
}

int operator==(record_data_t const & r1, record_data_t const & r2)
{
	if (r1.index != r2.index) {
		return false;
	}
	if (trace_cmp(&r1.record, &r2.record) != 0) {
		return false;
	}
	return true;
}

struct trace_record_cmp:
	public binary_function<record_data_t, record_data_t, bool> 
{
	bool operator()(
		record_data_t const & t1, 
		record_data_t const & t2
		)
	{
		//ignore index in comparison
		if (trace_cmp(&t1.record, &t2.record) < 0) {
			return true;
		}
		return false;
	}
};

/**
	Returns true is ranges overlap or are subsequent.
*/
int ranges_overlap(pgoff_t start1, pgoff_t len1, pgoff_t start2, pgoff_t len2)
{
	if (start1 + len1 < start2)
		return 0;
	if (start2 + len2 < start1)
		return 0;
	return 1;
}

void ranges_overlap_test(bool result, pgoff_t start1, pgoff_t len1, pgoff_t start2, pgoff_t len2)
{
	int r = ranges_overlap(start1, len1, start2, len2);
	if (r && result != true) {
		fprintf(stderr, "ranges_overlap test failed: %ld %ld %ld %ld\n", start1, len1, start2, len2);
		exit(7);
	}
	
	//now reverse, as this function should be symmetric
	r = ranges_overlap(start2, len2, start1, len1);
	if (r && result != true) {
		fprintf(stderr, "ranges_overlap test failed: %ld %ld %ld %ld\n", start2, len2, start1, len1);
		exit(7);
	}
}

void unittest_ranges_overlap()
{
	//subsequent ranges
	ranges_overlap_test(true, 0, 1, 1, 1);
	ranges_overlap_test(true, 0, 2, 2, 2);
	ranges_overlap_test(true, 1, 2, 3, 1);
	
	//overlap in the middle
	ranges_overlap_test(true, 1, 3, 3, 5); //to beginning
	ranges_overlap_test(true, 1, 3, 2, 7); //completely inside
	ranges_overlap_test(true, 1, 6, 3, 4); //to the end
	
	//overlap inside
	ranges_overlap_test(true, 1, 1, 1, 7); //to beginning
	ranges_overlap_test(true, 2, 2, 1, 7); //completely inside
	ranges_overlap_test(true, 2, 2, 0, 4); //to the end
	
	//no overlapping
	ranges_overlap_test(false, 1, 2, 4, 1);
	ranges_overlap_test(false, 1, 1, 3, 7);
	ranges_overlap_test(false, 1, 1, 6, 3);
	ranges_overlap_test(false, 10, 10, 22, 222);
}

/**
	Merges consecutive records of trace if they cover subsequent or overlapping range.
*/
void trace_merge(records_container_t &trace_container, records_container_t::iterator const & begin, records_container_t::iterator const & end)
{
	records_container_t::iterator it;
	records_container_t output;
	records_container_t::iterator current = end; //currently merged record
	int new_data_pending = 0;
	
	for (it = begin; it != end; ++it) {
		if (current == end) {
			current = it;
			continue;
		}
		
		prefetch_trace_record_t *r1 = &current->record;
		prefetch_trace_record_t *r2 = &it->record;
		
		if (r1->device != r2->device
			|| r1->inode_no != r2->inode_no
			|| ! ranges_overlap(r1->range_start, r1->range_length, r2->range_start, r2->range_length)
			) {
			output.push_back(*current);
			current = it;
			continue;
		}
		
		//ranges overlap
		record_data_t new_data = *it; //copy in case some fields are added
		new_data.index = min(it->index, current->index);
		new_data.record.device = r1->device;
		new_data.record.inode_no = r1->inode_no;
		new_data.record.range_start = min(r1->range_start, r2->range_start);
		pgoff_t max_end = max(r1->range_start + r1->range_length, r2->range_start + r2->range_length);
		new_data.record.range_length = max_end - new_data.record.range_start;
		*current = new_data;
	}
	if (current != end) {
		//add last item we were hoping to merge
		output.push_back(*current);
	}
	output.swap(trace_container);
}

void print_trace_container(records_container_t const & c)
{
	records_container_t::const_iterator it;
	for (it = c.begin(); it != c.end(); ++it) {
		prefetch_trace_record_t const & record = it->record;
		fprintf(stderr, "%d %u:%u\t%lu\t%lu\t%lu\n",
			it->index,
			MAJOR(record.device), MINOR(record.device),
			record.inode_no,
			record.range_start, record.range_length
			);
	}
}

void trace_merge_test(
	char *test_name,
	records_container_t const & input, 
	records_container_t const & expected_result
	)
{
	records_container_t input_copy = input;
	trace_merge(input_copy, input_copy.begin(), input_copy.end());
	if (! (expected_result == input_copy)) {
		fprintf(stderr, "Trace merge test %s failed\n", test_name);
		fprintf(stderr, "Input:\n");
		print_trace_container(input);
		fprintf(stderr, "Result:\n");
		print_trace_container(input_copy);
		fprintf(stderr, "Expected result:\n");
		print_trace_container(expected_result);
	}
}

typedef struct {
	kdev_t device;
	unsigned long inode_no;
} test_record_data_t;

void push_test_record(
	records_container_t & container, 
	test_record_data_t &other_data, 
	int index,
	pgoff_t start, 
	pgoff_t len
	)
{
	record_data_t data;
	data.index = index;
	data.record.device = other_data.device;
	data.record.inode_no = other_data.inode_no;
	data.record.range_start = start;
	data.record.range_length = len;
	container.push_back(data);
}

/**
	Unittests for trace_merge().
*/
void unittest_trace_merge()
{
	test_record_data_t inode1_7 = {
		1, //device
		7 //inode_no
	};
	test_record_data_t inode1_8 = {
		1, //device
		8 //inode_no
	};
	test_record_data_t inode2_7 = {
		2, //device
		7 //inode_no
	};
	
	//empty trace
	{
		records_container_t input;
		trace_merge_test("Empty trace", input, input);
	}
	//one sole record
	{
		records_container_t input;
		//input
		push_test_record(input, inode1_7, 0, 0, 1);
		
		trace_merge_test("One record", input, input);
	}
	//two unrelated records in the same inode
	{
		records_container_t input;
		//input
		push_test_record(input, inode1_7, 0, 1, 2);
		push_test_record(input, inode1_7, 1, 4, 1);
		
		trace_merge_test("Two unrelated in the same inode", input, input);
	}
	//two unrelated records with overlapping ranges in the different inodes
	{
		records_container_t input;
		//input
		push_test_record(input, inode1_7, 0, 1, 2);
		push_test_record(input, inode1_8, 1, 1, 2);
		
		trace_merge_test("Two unrelated in different inode", input, input);
	}
	//two unrelated records with overlapping ranges in the different devices and the same inode
	{
		records_container_t input;
		//input
		push_test_record(input, inode1_7, 0, 1, 2);
		push_test_record(input, inode2_7, 1, 1, 2);
		
		trace_merge_test("Two unrelated in different device, the same inode", input, input);
	}
	{
		records_container_t input;
		records_container_t expected_result;
		//input
		push_test_record(input, inode1_7, 0, 1, 1);
		push_test_record(input, inode1_7, 1, 2, 1);
		//expected_result
		push_test_record(expected_result, inode1_7, 0, 1, 2);
		
		trace_merge_test("Two consecutive merged", input, expected_result);
	}
	{
		records_container_t input;
		records_container_t expected_result;
		//input
		push_test_record(input, inode1_7, 0, 1, 1);
		push_test_record(input, inode1_7, 1, 2, 1);
		push_test_record(input, inode1_7, 2, 3, 2);
		//expected_result
		push_test_record(expected_result, inode1_7, 0, 1, 4);
		
		trace_merge_test("Three consecutive merged", input, expected_result);
	}
	{
		records_container_t input;
		records_container_t expected_result;
		//input
		push_test_record(input, inode1_7, 0, 1, 2);
		push_test_record(input, inode1_7, 1, 2, 2);
		push_test_record(input, inode1_7, 2, 3, 2);
		//expected_result
		push_test_record(expected_result, inode1_7, 0, 1, 4);
		
		trace_merge_test("Three overlapping merged", input, expected_result);
	}
	{
		records_container_t input;
		records_container_t expected_result;
		//input
		push_test_record(input, inode1_7, 0, 0, 2);
		push_test_record(input, inode1_7, 1, 4, 1);
		push_test_record(input, inode1_7, 2, 5, 1);
		//expected_result
		push_test_record(expected_result, inode1_7, 0, 0, 2);
		push_test_record(expected_result, inode1_7, 1, 4, 2);
		
		trace_merge_test("Two consecutive merged with not merged before", input, expected_result);
	}
	{
		records_container_t input;
		records_container_t expected_result;
		//input
		push_test_record(input, inode1_7, 0, 4, 1);
		push_test_record(input, inode1_7, 1, 5, 1);
		push_test_record(input, inode1_7, 2, 10, 2);
		//expected_result
		push_test_record(expected_result, inode1_7, 0, 4, 2);
		push_test_record(expected_result, inode1_7, 2, 10, 2);
		
		trace_merge_test("Two consecutive merged with not merged after", input, expected_result);
	}
}

/**
	Sorts the given trace file data and merges subsequent records.
*/
void trace_file_data_sorted_sum(trace_file_data_t *data)
{
	trace_record_cmp cmp;
	
	sort(data->records.begin(), data->records.end(), cmp);
	trace_merge(data->records, data->records.begin(), data->records.end());
}

/**
	Print @data structure as trace file to stdout.
	Returns 0 if success, <0 in case of error.
*/
int output_trace_file_data(trace_file_data_t *data)
{
	int i;
	int r;
	int record_size = sizeof(data->records[0].record);
	
	for (i = 0; i < data->records.size(); ++i) {
		r = fwrite(&data->records[i].record, record_size, 1, stdout);
		if (ferror(stdout)) {
			fprintf(stderr, "Error writing to stdout\n");
			return -1;
		}
		if (r == 0)
			break;
	}
	return 0;
}

void usage()
{
	printf("Usage: prefetch-process-trace [options] -c command file1 ... \n");
	printf("Result is output on stdout\n");
	printf("Available commands:\n");
	printf("\tsort-sum - concatenates, sorts and merges all input files\n");
}

enum {
	UNITTESTS = 1234,
};

int main(int argc, char **argv)
{
	prefetch_trace_record_t record;
	int r;
	int fd;
	char *filename;
	prefetch_trace_desc_t tracefile;
	int c;
	char *command = NULL;
	
	struct option long_options[] = {
		{"help", 0, 0, 0},
		{"unittests", 0, 0, UNITTESTS},
		{"command", 1, 0, 'c'},
		{0, 0, 0, 0}
	};

	for (;;)
	{
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		
		c = getopt_long(argc, argv, "hc:", long_options, &option_index);
		if (c == -1)
			break;
		
		switch (c) {
			case 'c':
				if (command != NULL) {
					fprintf(stderr, "Error: command specified more than once\n");
					usage();
					exit(1);
				}
				command = strdup(optarg);
				if (command == NULL) {
					fprintf(stderr, "Error: cannot allocate memory\n");
					exit(1);
				}
				break;
			case 'h':
				usage();
				exit(0);
			case UNITTESTS:
				unittest_ranges_overlap();
				unittest_trace_merge();
				printf("Unittests finished\n");
				exit(0);
				break;
			case ':':
				fprintf(stderr, "Error: missing argument to option %s\n", argv[optind]);
				usage();
				exit(1);
				break;
			case '?':
			default:
				fprintf(stderr, "Error: unknown option %s\n", argv[optind]);
				usage();
				exit(1);
				break;
		}
	}
	
	if (command == NULL) {
		fprintf(stderr, "Error: no command specified\n");
		usage();
		exit(1);
	}
	
	if (strcmp(command, "sort-sum") == 0) {
		trace_file_data_t data;
		for (;optind < argc; ++optind) {
			filename = argv[optind];
			if (trace_file_read_data(filename, &data) < 0) {
				exit(1);
			}
			trace_file_data_sorted_sum(&data);
			if (output_trace_file_data(&data) < 0) {
				exit(1);
			}
		}
	} else {
		fprintf(stderr, "Error: invalid command %s\n", command);
		usage();
		exit(1);
	}
	return 0;
}
