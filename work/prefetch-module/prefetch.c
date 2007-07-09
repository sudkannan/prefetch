/*
 * linux/fs/proc/filecache.c
 *
 * Copyright (C) 2006 Fengguang Wu <wfg@ustc.edu>
 * Copyright (C) 2007 Krzysztof Lichota <lichota@mimuw.edu.pl>
 *
 * Based on filecache code by Fengguang Wu.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/radix-tree.h>
#include <linux/page-flags.h>
#include <linux/pagevec.h>
#include <linux/pagemap.h>
#include <linux/vmalloc.h>
#include <linux/writeback.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/sort.h>
#include <linux/file.h>
#include <linux/delayacct.h>
#include <linux/workqueue.h>


//Inode walk session
struct session {
	int     private_session;
	pgoff_t     next_offset;
	struct {
		unsigned long cursor;
		unsigned long origin;
		unsigned long size;
		struct inode **inodes;
	} ivec;
	struct {
		unsigned long pos;
		unsigned long i_state;
		struct inode *inode;
		struct inode *pinned_inode;
	} icur;
	int inodes_walked;
	int pages_walked;
	int pages_referenced;
	int pages_referenced_debug;
	int page_blocks;
};

///Disables/enables the whole module functionality
static int prefetching_module_enabled = 1;

///Disables/enables prefetching of data during application start
static int prefetch_enabled = 1;

///Whether prefetching should be synchronous or asynchronous
///i.e. wait for prefetch to finish before going forward or not.
static int async_prefetching = 0;

enum
{
    PREFETCH_TRACE_SIZE = 128*1024,
    TRACING_INTERVAL_SECS = 2,
    TRACING_INTERVALS_NUM = 6, //12 seconds of tracing
};

struct prefetch_trace_t {
	unsigned int buffer_used;
	unsigned int buffer_size;
	void *buffer;
	spinlock_t prefetch_trace_lock;
	int overflow_reported;
};

struct prefetch_trace_t prefetch_trace = {
	0,
	0,
	NULL,
	SPIN_LOCK_UNLOCKED,
	0
};

typedef struct {
	dev_t device;
	unsigned long inode_no;
	pgoff_t range_start;
	pgoff_t range_length;
} prefetch_trace_record_t;

typedef struct {
	struct timespec ts_start;
	struct timespec ts_end;
	char *name;
} prefetch_timer_t;

/**
	Number of traces started and not finished.
*/
atomic_t tracers_count = ATOMIC_INIT(0);

/**
	Set if walk_pages() decided that it is the start of tracing
	and bits should be cleared, not recorded.
	Using it is protected by inode_lock.
	If lock breaking is enabled, this variable makes sure that
	second caller of walk_pages(START_TRACING) will not
	race with first caller and will not start recording changes.
*/
int clearing_in_progress = 0;

void clear_trace(void);

void prefetch_start_timing(prefetch_timer_t *timer, char *name)
{
	timer->name = name;
	do_posix_clock_monotonic_gettime(&timer->ts_start);
}

void prefetch_end_timing(prefetch_timer_t *timer)
{
	do_posix_clock_monotonic_gettime(&timer->ts_end);
}

void prefetch_print_timing(prefetch_timer_t *timer)
{
	struct timespec ts = timespec_sub(timer->ts_end, timer->ts_start);
	s64 ns = timespec_to_ns(&ts);

	printk(KERN_INFO "Prefetch timing (%s): %lld ns, %ld.%.9ld\n", timer->name, ns, ts.tv_sec, ts.tv_nsec);
}

typedef struct {
	void *trace;
	int trace_size;
} async_prefetch_params_t;

int prefetch_do_prefetch(void *trace, int trace_size);

static int async_prefetch_thread(void *p)
{
	int ret;
	async_prefetch_params_t *params = (async_prefetch_params_t *)p;
	printk(KERN_INFO "Started async prefetch thread\n");
	ret = prefetch_do_prefetch(params->trace, params->trace_size);
	kfree(params);
	return ret;
}

static int prefetch_start_prefetch_async(void *trace, int trace_size)
{
	async_prefetch_params_t *params = kmalloc(sizeof(async_prefetch_params_t), GFP_KERNEL);
	if (params == NULL)
		return -ENOMEM;
	params->trace = trace;
	params->trace_size = trace_size;
	
	if (kernel_thread(async_prefetch_thread, params, 0) < 0) {
		printk(KERN_WARNING "Cannot start async prefetch thread\n");
		return -EINVAL;
	}
	return 0;
}

static int prefetch_start_prefetch_sync(void *trace, int trace_size)
{
    return prefetch_do_prefetch(trace, trace_size);
}

int prefetch_start_prefetch(void *trace, int trace_size, int async)
{
    if (async) {
        return prefetch_start_prefetch_async(trace, trace_size);
    } else {
        return prefetch_start_prefetch_sync(trace, trace_size);
    }
}

int prefetch_do_prefetch(void *trace, int trace_size)
{
	prefetch_trace_record_t *record = trace;
	prefetch_trace_record_t *prev_record = NULL;
	prefetch_timer_t timer;
	struct super_block *sb = NULL;
	struct file *file = NULL;
	struct inode *inode = NULL;
	int ret = 0;
	int readahead_ret = 0;
	printk(KERN_INFO "Delay io ticks before prefetching: %d\n", (int)delayacct_blkio_ticks(current));
	
	prefetch_start_timing(&timer, "Prefetching");
	for (;(void *)(record + sizeof(prefetch_trace_record_t)) <= trace + trace_size;
		prev_record = record, ++record) {
		if (prev_record == NULL || prev_record->device != record->device) {
			//open next device
			if (sb)
				drop_super(sb);
			sb = user_get_super(record->device);
		}
		if (sb == NULL)
			continue; //no such device or error getting device

		if (prev_record == NULL || prev_record->device != record->device || prev_record->inode_no != record->inode_no) {
			//open next file
			if (inode)
				iput(inode);
			
			inode = iget(sb, record->inode_no);
			if (IS_ERR(inode)) {
				//no such inode or other error
				inode = NULL;
				continue;
			}
			
			if (file)
				put_filp(file);
			
			file = get_empty_filp();
			if (file == NULL) {
				ret = -ENFILE;
				goto out;
			}
			//only most important file fields filled, ext3_readpages doesn't use it anyway.
			file->f_op = inode->i_fop;
			file->f_mapping = inode->i_mapping;
			file->f_mode = FMODE_READ;
			file->f_flags = O_RDONLY;
// 			file->f_path.dentry = d_alloc(pfmfs_mnt->mnt_sb->s_root, &this);

/*	filp->f_path.mnt 	   = mntget(uverbs_event_mnt);
	filp->f_path.dentry 	   = dget(uverbs_event_mnt->mnt_root);*/
			
/*	d_instantiate(dentry, inode);
	inode->i_size = size;
	inode->i_nlink = 0;
	file->f_path.mnt = mntget(hugetlbfs_vfsmount);
	file->f_path.dentry = dentry;
	file->f_mapping = inode->i_mapping;
	file->f_mode = FMODE_WRITE | FMODE_READ;*/
// 		filp->f_op = &inotify_fops;
// 	filp->f_path.mnt = mntget(inotify_mnt);
// 	filp->f_path.dentry = dget(inotify_mnt->mnt_root);
// 	filp->private_data = dev;

		}
		if (inode == NULL)
			continue;
		//FIXME: use force_page_cache_readahead()?
 		readahead_ret = force_page_cache_readahead(inode->i_mapping, file, record->range_start, record->range_length);
	}

out:
	if (readahead_ret < 0) {
		printk(KERN_WARNING "Readahead caused some error, error=%d", readahead_ret);
	}
	if (sb)
		drop_super(sb);
	if (inode)
		iput(inode);
	if (file)
		put_filp(file);
	
	printk(KERN_INFO "Delay io ticks after prefetching: %d\n", (int)delayacct_blkio_ticks(current));
	prefetch_end_timing(&timer);
	prefetch_print_timing(&timer);
	return ret;
}


void prefetch_trace_add(
	dev_t device,
	unsigned long inode_no,
	pgoff_t range_start,
	pgoff_t range_length
	)
{
	int overflow_reported = 0;
	prefetch_trace_record_t *record;
	
	spin_lock(&prefetch_trace.prefetch_trace_lock);

	if (prefetch_trace.buffer_used + sizeof(prefetch_trace_record_t) >= prefetch_trace.buffer_size) {
		overflow_reported = prefetch_trace.overflow_reported;
		prefetch_trace.overflow_reported = 1;
		spin_unlock(&prefetch_trace.prefetch_trace_lock);
		
		if (!overflow_reported)
			printk(KERN_WARNING "Prefetch buffer overflow\n");
		return;
	}

	record = (prefetch_trace_record_t *)(prefetch_trace.buffer + prefetch_trace.buffer_used);
	prefetch_trace.buffer_used += sizeof(prefetch_trace_record_t);

	record->device = device;
	record->inode_no = inode_no;
	record->range_start = range_start;
	record->range_length = range_length;
	spin_unlock(&prefetch_trace.prefetch_trace_lock);
}

#define IVEC_SIZE	(PAGE_SIZE / sizeof(struct inode *))

/*
 * Full: there are more data following.
 */
static int ivec_full(struct session *s)
{
	return !s->ivec.cursor ||
	       s->ivec.cursor > s->ivec.origin + s->ivec.size;
}

static int ivec_push(struct session *s, struct inode *inode)
{
	if (!atomic_read(&inode->i_count))
		return 0;
	if (inode->i_state & (I_FREEING|I_CLEAR|I_WILL_FREE))
		return 0;
	if (!inode->i_mapping)
		return 0;

	s->ivec.cursor++;

	if (s->ivec.size >= IVEC_SIZE)
		return 1;

	if (s->ivec.cursor > s->ivec.origin)
		s->ivec.inodes[s->ivec.size++] = inode;
	return 0;
}

/*
 * Travease the inode lists in order - newest first.
 * And fill @s->ivec.inodes with inodes positioned in [@pos, @pos+IVEC_SIZE).
 */
static int ivec_fill(struct session *s, unsigned long pos)
{
	struct inode *inode;
	struct super_block *sb;

	s->ivec.origin = pos;
	s->ivec.cursor = 0;
	s->ivec.size = 0;

	/*
	 * We have a cursor inode, clean and expected to be unchanged.
	 */
	if (s->icur.inode && pos >= s->icur.pos &&
	        !(s->icur.i_state & I_DIRTY) &&
	        s->icur.i_state == s->icur.inode->i_state) {
		inode = s->icur.inode;
		s->ivec.cursor = s->icur.pos;
		goto continue_from_saved;
	}

	spin_lock(&sb_lock);
	list_for_each_entry(sb, &super_blocks, s_list) {
		list_for_each_entry(inode, &sb->s_dirty, i_list) {
			if (ivec_push(s, inode))
				goto out_full_unlock;
		}
		list_for_each_entry(inode, &sb->s_io, i_list) {
			if (ivec_push(s, inode))
				goto out_full_unlock;
		}
	}
	spin_unlock(&sb_lock);

	list_for_each_entry(inode, &inode_in_use, i_list) {
		if (ivec_push(s, inode))
			goto out_full;
continue_from_saved:
		;
	}

	list_for_each_entry(inode, &inode_unused, i_list) {
		if (ivec_push(s, inode))
			goto out_full;
	}

	return 0;

out_full_unlock:
	spin_unlock(&sb_lock);
out_full:
	return 1;
}

static struct inode *ivec_inode(struct session *s, unsigned long pos)
{
	if ((ivec_full(s) && pos >= s->ivec.origin + s->ivec.size)
	        || pos < s->ivec.origin)
		ivec_fill(s, pos);

	if (pos >= s->ivec.cursor)
		return NULL;

	s->icur.pos = pos;
	s->icur.inode = s->ivec.inodes[pos - s->ivec.origin];
	return s->icur.inode;
}

void add_referenced_page_range(struct session *s,
       struct address_space *mapping, pgoff_t start, pgoff_t len)
{
	struct inode *inode;
/*	if (len <= 0)
		return;*/
	s->pages_referenced += len;
	s->page_blocks++;
	if (!clearing_in_progress) {
		inode = mapping->host;
		if (inode && inode->i_sb && inode->i_sb->s_bdev)
			prefetch_trace_add(inode->i_sb->s_bdev->bd_dev, inode->i_ino, start, len);
	}
}

static void walk_file_cache(struct session *s,
        struct address_space *mapping)
{
	int i;
	pgoff_t len = 0;
	struct pagevec pvec;
	struct page *page;
	struct page *page0 = NULL;
	int current_page_referenced = 0;
	int previous_page_referenced = 0;
	pgoff_t start = 0;

	for (;;) {
		pagevec_init(&pvec, 0);
		pvec.nr = radix_tree_gang_lookup(&mapping->page_tree,
		                                 (void **)pvec.pages, start + len, PAGEVEC_SIZE);

		if (pvec.nr == 0) {
			//no more pages present
			//add the last range, if present
			if (previous_page_referenced)
				add_referenced_page_range(s, mapping, start, len);
			goto out;
		}

		if (!page0) {
			page0 = pvec.pages[0];
			previous_page_referenced = PageReferenced(page0);
		}

		for (i = 0; i < pvec.nr; i++) {

			page = pvec.pages[i];
			current_page_referenced = TestClearPageReferenced(page);

			s->pages_walked++;
			if (current_page_referenced)
				s->pages_referenced_debug++; //for debugging, to make sure we process all in add_referenced_page_range()

			if (page->index == start + len && previous_page_referenced == current_page_referenced)
				len++;
			else {
				if (previous_page_referenced)
					add_referenced_page_range(s, mapping, start, len);

				page0 = page;
				start = page->index;
				len = 1;
			}
			previous_page_referenced = current_page_referenced;
		}
	}

out:
	return;
}


static void show_inode(struct session *s, struct inode *inode)
{
	++s->inodes_walked; //just for stats, thus not using atomic_inc()

	if (inode->i_mapping) {
		walk_file_cache(s, inode->i_mapping);
	}
}

void prefetch_print_trace_fragment(struct seq_file *m,
	void *trace_fragment, int trace_size
	)
{
	prefetch_trace_record_t *record;
	
	for (
	    record = (prefetch_trace_record_t*)trace_fragment;
	    record < (prefetch_trace_record_t*)(trace_fragment + trace_size);
	    ++record
		) {
		seq_printf(m, "%u:%u\t%lu\t%lu\t%lu\n",
			MAJOR(record->device), MINOR(record->device),
			record->inode_no,
			record->range_start, record->range_length
			);
	}
}

void print_prefetch_trace(struct seq_file *m)
{
	void *trace_copy;
	unsigned int trace_size;

	trace_copy = kmalloc(PREFETCH_TRACE_SIZE, GFP_KERNEL);
	if (trace_copy == NULL) {
		printk(KERN_WARNING "Cannot allocate memory for prefetch trace copy");
		return;
	};

	spin_lock(&prefetch_trace.prefetch_trace_lock);

	if (prefetch_trace.buffer_used > PREFETCH_TRACE_SIZE) {
		//this shouldn't happen
		goto out_unlock_free;
	}
	trace_size = prefetch_trace.buffer_used;
	memcpy(trace_copy, prefetch_trace.buffer, prefetch_trace.buffer_used);
	spin_unlock(&prefetch_trace.prefetch_trace_lock);

	prefetch_print_trace_fragment(m, trace_copy,  trace_size);

	kfree(trace_copy);

	return;

out_unlock_free:
	spin_unlock(&prefetch_trace.prefetch_trace_lock);
	kfree(trace_copy);
	printk(KERN_WARNING "Printing prefetch trace failed");
}

//NOTE: this function is called with inode_lock spinlock held
static int inode_walk_show(struct session *s, loff_t pos)
{
	unsigned long index = pos;
	struct inode *inode;

	inode = ivec_inode(s,index);
	BUG_ON(!inode);
	show_inode(s, inode);

	return 0;
}

static void *inode_walk_start(struct session *s, loff_t *pos)
{
	s->ivec.inodes = (struct inode **)__get_free_page(GFP_KERNEL);
	if (!s->ivec.inodes)
		return NULL;
	s->ivec.size = 0;

	spin_lock(&inode_lock);

	BUG_ON(s->icur.pinned_inode);
	s->icur.pinned_inode = s->icur.inode;
	return ivec_inode(s, *pos) ? pos : NULL;
}

static void inode_walk_stop(struct session *s)
{
	if (s->icur.inode) {
		__iget(s->icur.inode);
		s->icur.i_state = s->icur.inode->i_state;
	}

	spin_unlock(&inode_lock);
	free_page((unsigned long )s->ivec.inodes);

	if (s->icur.pinned_inode) {
		iput(s->icur.pinned_inode);
		s->icur.pinned_inode = NULL;
	}
}

//NOTE: this function is called with inode_lock spinlock held
static void *inode_walk_next(struct session *s, loff_t *pos)
{
	(*pos)++;

	return ivec_inode(s, *pos) ? pos : NULL;
}

static void *prefetch_proc_start(struct seq_file *m, loff_t *pos)
{
	if (*pos != 0)
		return NULL;
	
	return &prefetch_trace; //whatever pointer, must not be NULL
}

static void *prefetch_proc_next(struct seq_file *m, void *v, loff_t *pos)
{
	return NULL;
}

extern void *ooffice_trace;
extern int ooffice_trace_size;

static int prefetch_proc_show(struct seq_file *m, void *v)
{
	if (ooffice_trace != NULL) {
		//FIXME: hack for debugging
		//FIXME: race with deallocation possible
		seq_printf(m, "OpenOffice trace\n");
		prefetch_print_trace_fragment(m, ooffice_trace, ooffice_trace_size);
	}
	seq_printf(m, "prefetch trace\n");
	print_prefetch_trace(m);
	return 0;
}

static void prefetch_proc_stop(struct seq_file *m, void *v)
{
}


struct seq_operations seq_prefetch_op = {
	.start	= prefetch_proc_start,
	.next	= prefetch_proc_next,
	.stop	= prefetch_proc_stop,
	.show	= prefetch_proc_show,
 };

static struct session *session_create(void)
{
	struct session *s;
	int err = 0;

	s = kzalloc(sizeof(*s), GFP_KERNEL);
	if (!s)
		err = -ENOMEM;

	return err ? ERR_PTR(err) : s;
}

static void session_release(struct session *s)
{
	if (s->icur.inode)
		iput(s->icur.inode);
	kfree(s);
}

enum tracing_command_t {
	START_TRACING,
	STOP_TRACING,
	CONTINUE_TRACING
};

typedef struct {
	unsigned position;
	unsigned generation;
} trace_marker_t;

void print_marker(char *msg, trace_marker_t marker)
{
	printk("%s %u.%u\n", msg, marker.generation, marker.position);
}

/**
	Returns current trace marker.
	Note: marker ranges are open on the right side, i.e.
	[start_marker, end_marker)
*/
trace_marker_t get_trace_marker(void)
{
	trace_marker_t marker;
	
	spin_lock(&prefetch_trace.prefetch_trace_lock);
	marker.position = prefetch_trace.buffer_used;
	marker.generation = 0; //FIXME: add generations
	spin_unlock(&prefetch_trace.prefetch_trace_lock);
	
	return marker;
}

/**
	Returns size of prefetch trace between start and end marker.
	Returns <0 if error occurs.
*/
static int prefetch_trace_fragment_size(trace_marker_t start_marker, trace_marker_t end_marker)
{
	if (start_marker.generation != end_marker.generation)
		return -EINVAL; //trace must have wrapped around and trace is no longer available
	if (end_marker.position < start_marker.position)
		return -ERANGE; //invalid markers
	
	return end_marker.position - start_marker.position;
}

/**
	Returns position in trace buffer for given marker.
	prefetch_trace_lock spinlock must be held when calling this function.
	Returns < 0 in case of error.
	Returns -ENOSPC if this marker is not in buffer.
	Note: marker ranges are open on right side, so this position
	might point to first byte after the buffer for end markers.
*/
static int trace_marker_position_in_buffer(trace_marker_t marker)
{
	//FIXME: check generation
	if (prefetch_trace.buffer_used < marker.position) {
		return -ENOSPC;
	}
	//for now simple, not circular buffer
	return marker.position;
}

/**
	Fetches fragment of trace between start marker and end_marker.
	Returns memory allocated using kmalloc() which holds trace fragment
	or error on @fragment_result in case of success and its size on @fragment_size_result.
	If fragment_size == 0, fragment is NULL.
*/
int get_prefetch_trace_fragment(trace_marker_t start_marker,
	trace_marker_t end_marker, void **fragment_result, int *fragment_size_result)
{
	int start_position;
	int end_position;
	int len;
	int ret;
	void *fragment;
	int fragment_size;
	
	fragment_size = prefetch_trace_fragment_size(start_marker, end_marker);
	if (fragment_size < 0)
		return fragment_size;
	if (fragment_size == 0) {
		*fragment_size_result = 0;
		*fragment_result = NULL;
		return 0;
	}

	fragment = kmalloc(fragment_size, GFP_KERNEL);
	if (fragment == NULL)
		return -ENOMEM;
	
	spin_lock(&prefetch_trace.prefetch_trace_lock);
	
	start_position = trace_marker_position_in_buffer(start_marker);
	end_position = trace_marker_position_in_buffer(end_marker);
	
	if (start_position < 0) {
		ret = -ESRCH;
		goto out_free;
	}
	if (end_position < 0) {
		ret = -ESRCH;
		goto out_free;
	}
	
	len =  end_position - start_position;
	if (len <= 0 || len != fragment_size) {
		//FIXME: shouldn't happen, change to assertion
		ret = -ETXTBSY;
		goto out_free;
	}
	memcpy(fragment, prefetch_trace.buffer + start_position, len);
	
	spin_unlock(&prefetch_trace.prefetch_trace_lock);

	*fragment_result = fragment;
	*fragment_size_result = fragment_size;
	return 0;
	
out_free:
	spin_unlock(&prefetch_trace.prefetch_trace_lock);
	kfree(fragment);
	return ret;
}

int walk_pages(enum tracing_command_t command, trace_marker_t *marker)
{
	void *retptr;
	loff_t pos = 0;
	int ret;
	loff_t next;
	struct session *s;
	int clearing = 0;
	int invalid_trace_counter = 0;
	prefetch_timer_t walk_pages_timer;
	
	s = session_create();
	if (IS_ERR(s)) {
		retptr = s;
		goto out;
	}

	retptr = inode_walk_start(s, &pos);

	if (IS_ERR(retptr))
		goto out_error_session_release;
	
	//inode_lock spinlock held from here
	if (command == START_TRACING) {
		if (atomic_inc_return(&tracers_count) == 1) {
			//tracers_count was 0, this is first tracer, so just clear bits
			clearing = 1;
			clearing_in_progress = 1;
			*marker = get_trace_marker();
		}
	}

	if (!clearing) {
		prefetch_start_timing(&walk_pages_timer, "walk pages");
	}
	else
		prefetch_start_timing(&walk_pages_timer, "clearing pages");

	while (retptr != NULL) {
		//FIXME: add lock breaking
		ret = inode_walk_show(s, pos);
		if (ret < 0) {
			retptr = ERR_PTR(ret);
			goto out_error;
		}

		next = pos;
		retptr = inode_walk_next(s, &next);
		if (IS_ERR(retptr))
			goto out_error;
		pos = next;
	}
		
	if (command == STOP_TRACING) {
		if (atomic_dec_return(&tracers_count) < 0) {
			invalid_trace_counter = 1;
		}
		*marker = get_trace_marker();
	} else if (command == CONTINUE_TRACING) {
		*marker = get_trace_marker();
	}

out_error:
	if (clearing)
		clearing_in_progress = 0;

	inode_walk_stop(s);
	//inode_lock spinlock released
	if (clearing)
		printk(KERN_INFO "Clearing run finished\n");
	if (invalid_trace_counter)
		printk(KERN_WARNING "Trace counter is invalid\n");

	if (!IS_ERR(retptr)) {
		prefetch_end_timing(&walk_pages_timer);
		prefetch_print_timing(&walk_pages_timer);
		printk(KERN_INFO "Inodes walked: %d, pages walked: %d, referenced: %d"
			" referenced counted manually: %d, blocks: %d\n",
			s->inodes_walked, s->pages_walked, s->pages_referenced,
			s->pages_referenced_debug, s->page_blocks);
	}

out_error_session_release:
	session_release(s);
out:
	return PTR_ERR(retptr);
}

/**
	Starts tracing, if no error happens returns marker which points to start of trace on @marker.
*/
int prefetch_start_trace(trace_marker_t *marker)
{
	return walk_pages(START_TRACING, marker);
}

/**
	Performs interim tracing run, returns marker which points to current place in trace.
*/
int prefetch_continue_trace(trace_marker_t *marker)
{
	//the same as stop tracing, for now
	return walk_pages(CONTINUE_TRACING, marker);
}

/**
	Stops tracing, returns marker which points to end of trace.
*/
int prefetch_stop_trace(trace_marker_t *marker)
{
	return walk_pages(STOP_TRACING, marker);
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

void sort_trace_fragment(void *trace, int trace_size)
{
	sort(trace, trace_size/sizeof(prefetch_trace_record_t),
		sizeof(prefetch_trace_record_t), trace_cmp, NULL
		);
}

int ooffice_trace_in_progress = 0;
void *ooffice_trace = NULL;
int ooffice_trace_size = 0;
trace_marker_t ooffice_start_marker;
pid_t ooffice_pid;


/**
	Structure describing one tracing run for given application.
	Mutex @mutex prevents concurrent end of tracing and periodic tracing.
	It also protects members from concurrent changes.
*/
typedef struct {
	struct delayed_work work;
	struct mutex mutex; 
	int traces_left;
	int num_trace_offsets;
	int *trace_offsets; ///array of trace offsets in intervals
} ooffice_trace_work_t;

ooffice_trace_work_t ooffice_trace_work;

void ooffice_finish_tracing(void)
{
	trace_marker_t ooffice_end_marker;
	trace_marker_t ooffice_start_marker_copy;
	void *trace_fragment = NULL;
	int trace_fragment_size = 0;
	int ret;
	
	//use the work mutex for exclusiveness for now
	mutex_lock(&ooffice_trace_work.mutex);
	
	if (delayed_work_pending(&ooffice_trace_work.work))
		cancel_delayed_work(&ooffice_trace_work.work);
	
	ooffice_start_marker_copy = ooffice_start_marker;
	ret = prefetch_stop_trace(&ooffice_end_marker);
	ooffice_trace_in_progress = 0;
	if (ret < 0) {
		printk(KERN_WARNING "Failed to stop ooffice trace\n");
		goto out_unlock;
	}
	
	ret = get_prefetch_trace_fragment(
		ooffice_start_marker_copy,
		ooffice_end_marker,
		&trace_fragment,
		&trace_fragment_size
		);
	if (ret < 0) {
		printk(KERN_WARNING "Failed to fetch ooffice trace fragment\n");
		goto out_unlock;
	}

	clear_trace();

	if (trace_fragment_size <= 0) {
		printk(KERN_WARNING "Empty ooffice trace\n");
		goto out_unlock;
	}

	sort_trace_fragment(trace_fragment, trace_fragment_size);

	if (ooffice_trace != NULL) {
		//FIXME: race with deallocation possible
		kfree(ooffice_trace);
	}

	ooffice_trace_size = trace_fragment_size;
	ooffice_trace = trace_fragment;
	
out_unlock:
	mutex_unlock(&ooffice_trace_work.mutex);
}

/**
	Function called in scheduled intervals to perform tracing with better resolution.
*/
static void ooffice_interim_tracing(struct work_struct *workptr)
{
	trace_marker_t interim_marker;
	int offset;
	int ret;
	
	mutex_lock(&ooffice_trace_work.mutex);
	
	ooffice_trace_work.traces_left--;
	if (ooffice_trace_work.traces_left <= 0) {
		//finish tracing
		printk("End of tracing after scheduled number of intervals\n");
		mutex_unlock(&ooffice_trace_work.mutex);
		ooffice_finish_tracing();
		goto out;
	}
	
	if (ooffice_trace_work.trace_offsets == NULL) {
		printk("Trace offsets not allocated\n");
		goto out_unlock;
	}

	ret = prefetch_continue_trace(&interim_marker);
	if (ret != 0) {
		printk(KERN_WARNING "Interim trace error=%d\n", ret);
		goto out_unlock;
	}
	
	offset = prefetch_trace_fragment_size(ooffice_start_marker, interim_marker);
	
	ooffice_trace_work.trace_offsets[ooffice_trace_work.num_trace_offsets] = offset;
	ooffice_trace_work.num_trace_offsets++;
	//schedule next run
	schedule_delayed_work(&ooffice_trace_work.work, HZ*TRACING_INTERVAL_SECS);
	
out_unlock:
	mutex_unlock(&ooffice_trace_work.mutex);
out:
	return;
}

void prefetch_exec_hook(char *filename)
{
	int ret;

	if (strcmp(filename, "/usr/lib/openoffice/program/soffice.bin") == 0)
	{
		if (!prefetching_module_enabled) {
			printk("Prefetch disabled, not doing tracing nor prefetching\n");
			return;
		}
		
		printk(KERN_INFO "ooffice exec noticed\n");
		//NOTE: this is just proof-of-concept code, no locking is implemented

		if (ooffice_trace_in_progress == 0) {
			ret = prefetch_start_trace(&ooffice_start_marker);
			if (ret < 0) {
				printk(KERN_WARNING "Failed to start ooffice trace\n");
				return;
			}
			//FIXME: concurrent traces possible
			ooffice_trace_in_progress = 1;
			ooffice_pid = current->pid;

			//start delayed work
			mutex_lock(&ooffice_trace_work.mutex);
			
			if (ooffice_trace_work.trace_offsets != NULL)
				kfree(ooffice_trace_work.trace_offsets);
			ooffice_trace_work.traces_left = TRACING_INTERVALS_NUM;
			ooffice_trace_work.num_trace_offsets = 0;
			ooffice_trace_work.trace_offsets = kmalloc(ooffice_trace_work.traces_left *sizeof(ooffice_trace_work.trace_offsets[0]), GFP_KERNEL); //FIXME: ignoring memory not allocated for now
			schedule_delayed_work(&ooffice_trace_work.work, HZ*TRACING_INTERVAL_SECS);
			
			mutex_unlock(&ooffice_trace_work.mutex);
		}
		if (ooffice_trace != NULL) {
			if (prefetch_enabled) {
				printk("Starting prefetch\n");
				prefetch_start_prefetch(ooffice_trace, ooffice_trace_size, async_prefetching);
			} else {
				printk("Prefetch disabled\n");
			}
		}
	}
}

/**
	Prefetch hook for intercepting exit() of process.
*/
void prefetch_exit_hook(pid_t pid)
{
	if (ooffice_trace_in_progress && ooffice_pid == pid) {
		printk(KERN_INFO "ooffice exit noticed during tracing\n");
		ooffice_finish_tracing();
	}
}


/*
 * Proc file operations.
 */

static int prefetch_proc_open(struct inode *inode, struct file *proc_file)
{
	return  seq_open(proc_file, &seq_prefetch_op);
}

static int prefetch_proc_release(struct inode *inode, struct file *proc_file)
{
	return seq_release(inode, proc_file);
}

void clear_trace(void)
{
	printk(KERN_INFO "Clearing prefetch trace\n");

	spin_lock(&prefetch_trace.prefetch_trace_lock);

	if (prefetch_trace.buffer == NULL) {
		prefetch_trace.buffer = kmalloc(PREFETCH_TRACE_SIZE, GFP_KERNEL);
		if (prefetch_trace.buffer == NULL) {
			//failed to allocate memory
			goto out_unlock;
		}
		prefetch_trace.buffer_size = PREFETCH_TRACE_SIZE;
	}
	//reset used buffer counter
	prefetch_trace.buffer_used = 0;
	prefetch_trace.overflow_reported = 0;

out_unlock:
	spin_unlock(&prefetch_trace.prefetch_trace_lock);
}

static int param_match(char *line, char *param_name)
{
	if (strncmp(line, param_name, strlen(param_name)) == 0)
	{
		return 1;
	}
	return 0;
}

ssize_t prefetch_proc_write(struct file *proc_file, const char __user * buffer,
	size_t count, loff_t *ppos)
{
	char *name;
	int e = 0;
	trace_marker_t marker;

	if (count >= PATH_MAX)
		return -ENAMETOOLONG;

	name = kmalloc(count+1, GFP_KERNEL);
	if (!name)
		return -ENOMEM;

	if (copy_from_user(name, buffer, count)) {
		e = -EFAULT;
		goto out;
	}

	/* strip the optional newline */
	if (count && name[count-1] == '\n')
		name[count-1] = '\0';
	else
		name[count] = '\0';

	if (param_match(name, "clear trace")) {
		clear_trace();
		goto out;
	}
	
	if (param_match(name, "mode async")) {
		printk(KERN_INFO "Async prefetching mode set\n");
		async_prefetching = 1;
		goto out;
	}

	if (param_match(name, "mode sync")) {
		printk(KERN_INFO "Sync prefetching mode set\n");
		async_prefetching = 0;
		goto out;
	}

	if (param_match(name, "enable")) {
		printk(KERN_INFO "Prefetch module enabled\n");
		prefetching_module_enabled = 1;
		goto out;
	}
	
	if (param_match(name, "disable")) {
		printk(KERN_INFO "Prefetch module disabled\n");
		prefetching_module_enabled = 0;
		goto out;
	}
	
	if (param_match(name, "prefetch enable")) {
		printk(KERN_INFO "Prefetching enabled\n");
		prefetch_enabled = 1;
		goto out;
	}
	
	if (param_match(name, "prefetch disable")) {
		printk(KERN_INFO "Prefetching disabled\n");
		prefetch_enabled = 0;
		goto out;
	}
	
	if (param_match(name, "start")) {
		prefetch_start_trace(&marker);
		print_marker("Start marker: ", marker);
		goto out;
	}
	
	if (param_match(name, "stop")) {
		prefetch_stop_trace(&marker);
		print_marker("Stop marker: ", marker);
		goto out;
	}
out:
	kfree(name);

	return e ? e : count;
}

static struct file_operations proc_prefetch_fops = {
	.owner		= THIS_MODULE,
	.open		= prefetch_proc_open,
	.release		= prefetch_proc_release,
	.write		= prefetch_proc_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
};

static __init int prefetch_init(void)
{
	struct proc_dir_entry *entry;
	clear_trace();

	//initialize periodic work
	INIT_DELAYED_WORK(&ooffice_trace_work.work, ooffice_interim_tracing);
	mutex_init(&ooffice_trace_work.mutex);
	ooffice_trace_work.traces_left = 0; //just in case
	ooffice_trace_work.num_trace_offsets = 0;
	ooffice_trace_work.trace_offsets = NULL;

	//create proc entry
	entry = create_proc_entry("prefetch", 0600, NULL);
	if (entry)
		entry->proc_fops = &proc_prefetch_fops;

	return 0;
}

static void prefetch_exit(void)
{
	remove_proc_entry("prefetch", NULL);
}

MODULE_AUTHOR("Krzysztof Lichota <lichota@mimuw.edu.pl>");
MODULE_LICENSE("GPL");

module_init(prefetch_init);
module_exit(prefetch_exit);
