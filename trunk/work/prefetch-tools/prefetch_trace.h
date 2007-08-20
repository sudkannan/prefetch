#ifndef __PREFETCH_H
#define __PREFETCH_H

#define MINORBITS	20
#define MINORMASK	((1U << MINORBITS) - 1)

#define MAJOR(dev)	((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev)	((unsigned int) ((dev) & MINORMASK))

typedef unsigned int kdev_t;
typedef unsigned long pgoff_t;

typedef struct {
	kdev_t device;
	unsigned long inode_no;
	pgoff_t range_start;
	pgoff_t range_length;
} prefetch_trace_record_t;

#endif //__PREFETCH_H
