#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <et/com_err.h>
#include <ext2fs/ext2fs.h>
#include <asm/byteorder.h>

#define INODE_ALLOC_ENTRIES 8192
#define FREE_EXTENT_ALLOC_ENTRIES 1024

#define INODE_HASH_SIZE 8192		/* Default size of inode hash */
#define MIN_INODE_HASH_SIZE 256		/* Minimal size of inode hash */
#define MIN_FREE_EXTENT_SIZE 16		/* Minimal size of recorded free extent */

#define REMAP_BUF_BLOCKS 256		/* Number of filesystem blocks we remap at once */

#define INDIRECT_TO_OFF(offset, level) (-(((offset) << 2) | (level)))

/* Structure for extents to remap */
struct remap_extent {
	blk_t block;		/* Physical start of an extent */
	unsigned int len;	/* Length of an extent */
	e2_blkcnt_t offset;	/* Logical offset in file / dir */
	e2_blkcnt_t chunk_offset;	/* Desired position after relocation */
	struct remap_extent *l, *r;	/* Splay tree pointers */
	struct remap_extent *inode_next;	/* List of extents in the inode */
};

/* Structure keeping inode extents */
struct inode_extents {
	ino_t ino;
	struct remap_extent *extents;
	struct remap_extent *last_ext;
	struct inode_extents *next;
};

/* Extent of free blocks */
struct free_extent {
	blk_t block;
	blk_t len;
	blk_t offset;
};

/* Data for inode block traversal */
struct block_traverse_data {
	int tree_ind[2];	/* Index in the INDIRECT and DOUBLE_INDIRECT levels */
	int depth;		/* Depth of the tree we are currently searching */
	struct remap_extent *actext;	/* Current extent to remap */
	ext2_ino_t ino;
};

FILE *inp_file;		/* File with mapped extents */
ext2_filsys fs;		/* Filesystem to work on */
unsigned int extent_count;	/* Number of extents to remap */
struct remap_extent *extent_tree;	/* Tree of extents */
unsigned int inode_hash_size = INODE_HASH_SIZE;	/* Size of inode hash table */
struct inode_extents **inode_hash;	/* Hashtable of inodes */
unsigned int inode_count;	/* Number of inodes */
struct inode_extents *inodes;	/* Array of inodes */
e2_blkcnt_t remap_blocks;	/* Number of blocks needing remapping */
unsigned int free_extent_count;		/* Number of extents of free space */
struct free_extent *free_extents;	/* Array of extents of free space */
int min_free_extent_size = MIN_FREE_EXTENT_SIZE; 	/* Minimal size of extent of free blocks */
int remap_buf_blocks = REMAP_BUF_BLOCKS;	/* Number of blocks to remap at once */
int device_fd;				/* Open device with the filesystem */

/* An implementation of top-down splaying D. Sleator <sleator@cs.cmu.edu> */
struct remap_extent *extent_splay(blk_t blocknr)
{
	struct remap_extent N, *l, *r, *y;

	if (!extent_tree)
		return NULL;
	N.l = N.r = NULL;
	l = r = &N;

	for (;;) {
		if (blocknr < extent_tree->block) {
			if (!extent_tree->l)
				break;
			if (blocknr < extent_tree->l->block) {
				y = extent_tree->l;	   /* rotate right */
				extent_tree->l = y->r;
				y->r = extent_tree;
				extent_tree = y;
				if (!extent_tree->l)
					break;
			}
			r->l = extent_tree;		   /* link right */
			r = extent_tree;
			extent_tree = extent_tree->l;
		} else if (blocknr >= extent_tree->block+extent_tree->len) {
			if (!extent_tree->r)
				break;
			if (blocknr >= extent_tree->r->block+extent_tree->r->len) {
				y = extent_tree->r;	  /* rotate left */
				extent_tree->r = y->l;
				y->l = extent_tree;
				extent_tree = y;
				if (!extent_tree->r)
					break;
			}
			l->r = extent_tree;		  /* link left */
			l = extent_tree;
			extent_tree = extent_tree->r;
		} else
			break;
	}
	l->r = extent_tree->l;				/* assemble */
	r->l = extent_tree->r;
	extent_tree->l = N.r;
	extent_tree->r = N.l;
	return extent_tree;
}

void extent_insert(struct remap_extent *new)
{
	if (!extent_tree) {
		new->l = new->r = NULL;
		extent_tree = new;
		return;
	}
	extent_tree = extent_splay(new->block);
	if (new->block+new->len < extent_tree->block) {
		new->l = extent_tree->l;
		new->r = extent_tree;
		extent_tree->l = NULL;
	} else if (new->block >= extent_tree->block+extent_tree->len) {
		new->r = extent_tree->r;
		new->l = extent_tree;
		extent_tree->r = NULL;
	} else {
		/* We get here if it's already in the tree */
		fprintf(stderr, "Tried to add overlapping extent %Ld+%u!\n",
			(long long)new->block, new->len);
		exit(1);
	}
	extent_tree = new;
}

/* End of splay tree implementation */


void parse_options(int argc, char **argv)
{
	errcode_t error;
	int c, endarg;
	char *endc;

	while ((c = getopt(argc, argv, "hi:f:r:")) != -1) {
		switch (c) {
			case 'i':
				inode_hash_size = strtol(optarg, &endc, 10);
				if (*endc || inode_hash_size <= MIN_INODE_HASH_SIZE) {
					fprintf(stderr, "Illegal inode hash size %s. Must be at least %d.\n", optarg, MIN_INODE_HASH_SIZE);
					exit(1);
				}
				break;
			case 'f':
				min_free_extent_size = strtol(optarg, &endc, 10);
				if (*endc || min_free_extent_size < 1) {
					fprintf(stderr, "Illegal minimum free extent size %s.\n", optarg);
					exit(1);
				}
			case 'r':
				remap_buf_blocks = strtol(optarg, &endc, 10);
				if (*endc || remap_buf_blocks < 1) {
					fprintf(stderr, "Illegal number of blocks to remap at once %s.\n", optarg);
					exit(1);
				}
			case '?':
			case 'h':
usage:
				fputs("Usage: e2remapblocks [-i inode-hash-size] [-f min-free-extent-size] [-r remap-blocks-at-once] <file-with-mapped-extents> <device>\n", stderr);
				exit(1);
		}
	}

	if (argc - optind != 2)
		goto usage;
	endarg = optind;
	if (!strcmp(argv[endarg], "-"))
		inp_file = stdin;
	else
		if (!(inp_file = fopen(argv[endarg], "r"))) {
			perror("Cannot open file with mapped extents");
			exit(1);
		}
	if ((error = ext2fs_open(argv[endarg+1], EXT2_FLAG_RW, 0, 0, unix_io_manager, &fs))) {
		fprintf(stderr, "Cannot open device %s: %s\n", argv[endarg+1],
			error_message(error));
		exit(1);
	}
	if ((fs->super->s_state & EXT2_ERROR_FS) || !(fs->super->s_state & EXT2_VALID_FS)) {
		fputs("Filesystem is in inconsistent state. Refusing to remap blocks.\n", stderr);
		exit(1);
	}
	device_fd = open(argv[endarg+1], O_RDWR);
	if (device_fd < 0) {
		perror("Cannot open device with a filesystem");
		exit(1);
	}
}

/* Offsets of block trees in the inode */
#define EXT2_IND_OFFSET(fs) (EXT2_NDIR_BLOCKS)
#define EXT2_DIND_OFFSET(fs) (EXT2_IND_OFFSET(fs)+((fs)->blocksize>>2))
#define EXT2_TIND_OFFSET(fs) (EXT2_DIND_OFFSET(fs)+\
                                ((fs)->blocksize>>2)*((fs)->blocksize>>2))

#define min(a, b) ((a) < (b) ? (a) : (b))

loff_t index_to_offset(ext2_filsys fs, int depth, int *index)
{
	if (depth == 0)
		return EXT2_IND_OFFSET(fs);
	if (depth == 1)
		return EXT2_DIND_OFFSET(fs) + index[0]*(fs->blocksize>>2);
	if (depth == 2)
		return EXT2_TIND_OFFSET(fs) + index[0]*(fs->blocksize>>2) +
			index[1]*(fs->blocksize>>2)*(fs->blocksize>>2);
	return 0;
}

/* Stupid but for now... */
#define HASH_INO(ino) ((ino)%inode_hash_size)

struct inode_extents *create_new_inode(ino_t ino)
{
	static int allocated;

	if (inode_count >= allocated) {
		inodes = realloc(inodes, (allocated+INODE_ALLOC_ENTRIES)
			*sizeof(struct inode_extents));
		if (!inodes) {
			fputs("Not enough memory for inodes.\n", stderr);
			exit(1);
		}
		allocated += INODE_ALLOC_ENTRIES;
	}
	inodes[inode_count].ino = ino;
	inodes[inode_count].extents = NULL;
	inodes[inode_count].last_ext = NULL;
	inodes[inode_count].next = inode_hash[HASH_INO(ino)];
	inode_hash[HASH_INO(ino)] = inodes+inode_count;
	return inodes+inode_count++;
}

static inline struct inode_extents *lookup_inode(ino_t ino)
{
	struct inode_extents *act = inode_hash[HASH_INO(ino)];

	while (act && act->ino != ino)
		act = act->next;
	return act;
}

/* Check if we are allowed to remap the inode
 * (for example we don't want to remap the journal) */
static inline int good_inode(ext2_ino_t ino)
{
	return ino >= EXT2_FIRST_INO(fs->super);
}

void insert_new_extent(blk_t blocknr, unsigned int len, e2_blkcnt_t offset, ext2_ino_t ino)
{
	struct remap_extent *ext;
	struct inode_extents *iext;

	if (!good_inode(ino))
		return;
	ext = malloc(sizeof(struct remap_extent));
	if (!ext) {
		fputs("Not enough memory for remapped extents.\n", stderr);
		exit(1);
	}
	iext = lookup_inode(ino);
	if (!iext)
		iext = create_new_inode(ino);
	ext->block = blocknr;
	ext->len = len;
	ext->offset = offset;

	ext->inode_next = NULL;
	if (iext->last_ext)
		iext->last_ext->inode_next = ext;
	else
		iext->extents = ext;
	iext->last_ext = ext;

	ext->chunk_offset = remap_blocks;
	remap_blocks += ext->len;
	extent_insert(ext);
}

void load_extents(void)
{
	long long blocknr, offset;
	unsigned int len, ino;
	struct remap_extent *ext;
	char linebuf[256];
	char *c;

	inode_hash = malloc(inode_hash_size*sizeof(struct inode_extents *));
	if (!inode_hash) {
		fputs("Not enough memory for inode hash table.\n", stderr);
		exit(1);
	}
	while (fgets(linebuf, sizeof(linebuf), inp_file)) {
		c = strchr(linebuf, ' ');
		if (!c) {
parse_error:
			fputs("Cannot parse input file.\n", stderr);
			exit(1);
		}
		if (!strncmp(c+1, "unmapped", 8))
			continue;
		if (sscanf(linebuf, "%Ld+%u %Ld INO %u", &blocknr, &len, &offset, &ino) != 4)
			goto parse_error;
		while (len) {
			ext = extent_splay(blocknr);
			if (!ext) {
				insert_new_extent(blocknr, len, offset, ino);
				break;
			}
			if (blocknr < ext->block) {
				if (blocknr+len < ext->block) {
					insert_new_extent(blocknr, len, offset, ino);
					break;
				}
				/* Extent overlaps with the following one */
				insert_new_extent(blocknr, ext->block-blocknr, offset, ino);
				fprintf(stderr, "Extent overlap found in extent starting at %Ld.\n", blocknr);
				offset += ext->block-blocknr;
				len -= ext->block-blocknr;
				blocknr = ext->block;
			}
			/* Block is inside some other extent? */
			if (ext->block <= blocknr && ext->block+ext->len > blocknr) {
				/* Completely inside? */
				if (ext->block+ext->len >= blocknr+len)
					break;
				fprintf(stderr, "Extent overlap found in extent starting at %Ld.\n", blocknr);
				len -= ext->block+ext->len-blocknr;
				offset += ext->block+ext->len-blocknr;
				blocknr = ext->block+ext->len;
			}
			else { /* We got extent before our block */
				/* No extent beyond the returned one? */
				if (!ext->r) {
					insert_new_extent(blocknr, len, offset, ino);
					break;
				}
				/* We let fall through to splay once more - now
				 * an extent after the blocknr should be returned... */
			}
		}
	}
	/* We no longer need the hash table */
	free(inode_hash);
	inode_hash = NULL;
}

#define OFF_TO_DEPTH(offset) ((-(offset)) & 3)
#define OFF_TO_DATAOFF(offset) ((-(offset)) >> 2)

inline int cmp_ext_offset_ptr_core(e2_blkcnt_t ao, e2_blkcnt_t bo)
{
	int a_depth = -1, b_depth = -1;

	if (ao < 0) {
		a_depth = OFF_TO_DEPTH(ao);
		ao = OFF_TO_DATAOFF(ao);
	}
	if (bo < 0) {
		b_depth = OFF_TO_DEPTH(bo);
		bo = OFF_TO_DATAOFF(bo);
	}
	if (ao < bo)
		return -1;
	if (ao > bo)
		return 1;
	/* Depths are ordered inversely */
	return b_depth - a_depth;
}

int cmp_ext_offset_ptr(const void *a, const void *b)
{
	e2_blkcnt_t ao = (*(struct remap_extent **)a)->offset,
		    bo = (*(struct remap_extent **)b)->offset;

	return cmp_ext_offset_ptr_core(ao, bo);
}

/* Sort extents of each inode according to offset in file */
void sort_inodes_extents(void)
{
	struct remap_extent **exts = NULL;
	unsigned int exts_count = 0;
	struct remap_extent *act_ext;
	unsigned int act_count;
	unsigned int i, j;

	for (i = 0; i < inode_count; i++) {
		/* Count extents */
		act_count = 0;
		for (act_ext = inodes[i].extents; act_ext; act_ext = act_ext->inode_next)
			act_count++;
		if (act_count > exts_count) {
			exts = realloc(exts, sizeof(struct remap_extent *)*act_count);
			if (!exts) {
				fputs("Not enough memory for inode extent array.\n", stderr);
				exit(1);
			}
			exts_count = act_count;
		}
		/* Store extent pointers into array */
		act_count = 0;
		for (act_ext = inodes[i].extents; act_ext; act_ext = act_ext->inode_next)
			exts[act_count++] = act_ext;
		qsort(exts, act_count, sizeof(struct remap_extent *), cmp_ext_offset_ptr);
		/* Relink link list according to new ordering */
		for (j = 0; j < act_count-1; j++)
			exts[j]->inode_next = exts[j+1];
		exts[act_count-1]->inode_next = NULL;
		inodes[i].extents = exts[0];
	}
}

int cmp_extents_len(const void *a, const void *b)
{
	if (((struct free_extent *)a)->len < ((struct free_extent *)b)->len)
		return -1;
	if (((struct free_extent *)a)->len > ((struct free_extent *)b)->len)
		return 1;
	return 0;
}

int cmp_extents_start(const void *a, const void *b)
{
	if (((struct free_extent *)a)->block < ((struct free_extent *)b)->block)
		return -1;
	if (((struct free_extent *)a)->block > ((struct free_extent *)b)->block)
		return 1;
	return 0;
}

/* Find free blocks where we could remap our data */
void find_free_blocks(void)
{
	errcode_t error = ext2fs_read_block_bitmap(fs);
	blk_t actblk;
	int allocated, actext, taken_extents;
	blk_t min_ext_size;

	if (error) {
		fprintf(stderr, "Cannot read block bitmap: %s\n", error_message(error));
		exit(1);
	}
	free_extents = malloc(sizeof(struct free_extent)*FREE_EXTENT_ALLOC_ENTRIES);
	if (!free_extents) {
		fputs("Not enough memory for free extents map.\n", stderr);
		exit(1);
	}
	allocated = FREE_EXTENT_ALLOC_ENTRIES;
	free_extents[0].block = free_extents[0].len = 0;
	actblk = ext2fs_get_block_bitmap_start(fs->block_map);
	while (actblk < ext2fs_get_block_bitmap_end(fs->block_map)) {
		/* Find a free block aligned at 8 blocks */
		actblk = (actblk + 8) & ~7;
		while (ext2fs_fast_test_block_bitmap(fs->block_map, actblk) &&
			actblk < ext2fs_get_block_bitmap_end(fs->block_map))
			actblk += 8;
		if (actblk >= ext2fs_get_block_bitmap_end(fs->block_map))
			break;
		free_extents[free_extent_count].block = actblk;
		while (!ext2fs_fast_test_block_bitmap(fs->block_map, actblk) &&
			actblk < ext2fs_get_block_bitmap_end(fs->block_map))
			actblk++;
		free_extents[free_extent_count].len = actblk -
			free_extents[free_extent_count].block;
		/* Found a single free chunk large enough? */
		if (free_extents[free_extent_count].len > remap_blocks) {
			free_extents[0] = free_extents[free_extent_count];
			free_extents[0].offset = 0;
			free_extents[0].len = remap_blocks;
			free_extent_count = 1;
			fprintf(stderr, "Found a single extent for all the data (%u blocks).\n", (unsigned)remap_blocks);
			return;
		}
		if (free_extents[free_extent_count].len >= min_free_extent_size) {
			free_extent_count++;
			if (free_extent_count >= allocated) {
				allocated += FREE_EXTENT_ALLOC_ENTRIES;
				free_extents = realloc(free_extents,
					sizeof(struct free_extent)*allocated);
				if (!free_extents) {
					fputs("Not enough memory for free extents map.\n", stderr);
					exit(1);
				}
			}
		}
	}
	qsort(free_extents, free_extent_count, sizeof(struct free_extent), cmp_extents_len);

	for (actblk = actext = 0; actext < free_extent_count && actblk < remap_blocks;
		actblk += free_extents[actext].len, actext++);
	if (actext >= free_extent_count) {
		fputs("Not enough free space for relocating required blocks (or maybe it is too fragmented).\n", stderr);
		exit(1);
	}
	/* OK, got enough free space. Now find suitable chunks */
	/* We'd accept extents of the size 1/2 of the needed to possibly improve
	 * locality */
	min_ext_size = free_extents[actext-1].len >> 1;
	qsort(free_extents, free_extent_count, sizeof(struct free_extent), cmp_extents_start);
	taken_extents = 0;
	for (actext = 0, actblk = 0; actext < free_extent_count &&
	     actblk < remap_blocks; actext++)
		if (free_extents[actext].len >= min_ext_size) {
			free_extents[actext].offset = actblk;
			actblk += free_extents[actext].len;
			free_extents[taken_extents++] = free_extents[actext];
		}
	/* Shorten the last extent accordingly */
	free_extents[taken_extents-1].len -= actblk - remap_blocks;
	/* Free memory used for unused extents */
	free_extent_count = taken_extents;
	free_extents = realloc(free_extents, free_extent_count*sizeof(struct free_extent));
	if (!free_extents) {
		fputs("Unable to shrink list of free extents.\n", stderr);
		exit(1);
	}
	fprintf(stderr, "Found %u extents, size threshold was %u blocks.\n",
		free_extent_count, min_ext_size);
}

void read_blocks(blk_t block, char *buf, blk_t len)
{
	ssize_t read_ret;

	read_ret = pread(device_fd, buf, ((ssize_t)len)*fs->blocksize, ((off_t)block)*fs->blocksize);
	if (read_ret != ((ssize_t)len)*fs->blocksize) {
		if (read_ret < 0)
			perror("Error while reading blocks from a device");
		else
			fprintf(stderr, "Short read from device at block %Ld, len %Ld\n", (long long) block, (long long)len);
		exit(1);
	}
}

void write_blocks(blk_t block, char *buf, blk_t len)
{
	ssize_t write_ret;

	write_ret = pwrite(device_fd, buf, ((ssize_t)len)*fs->blocksize, ((off_t)block)*fs->blocksize);
	if (write_ret != ((ssize_t)len)*fs->blocksize) {
		if (write_ret < 0)
			perror("Error while writing blocks to device");
		else
			fprintf(stderr, "Short write to device at block %Ld, len %Ld\n", (long long)block, (long long)len);
		exit(1);
	}
}

struct free_extent *find_remapped_offset(blk_t offset)
{
	int start, end, mid;

	start = 0;
	end = free_extent_count-1;

	while (start <= end) {
		mid = (start+end)/2;
		if (free_extents[mid].offset <= offset &&
		    free_extents[mid].offset+free_extents[mid].len > offset)
			return free_extents+mid;
		if (free_extents[mid].offset < offset)
			start = mid+1;
		else
			end = mid-1;
	}
	return NULL;
}

void copy_inode_blocks(struct inode_extents *inode)
{
	struct remap_extent *actext;
	static char *remap_buf;
	blk_t curlen, curpos, writepos, writelen;
	struct free_extent *freeext;

	if (!remap_buf) {
	 	remap_buf = malloc(fs->blocksize*remap_buf_blocks);
		if (!remap_buf) {
			fputs("Not enough memory for buffer for remapping blocks.\n", stderr);
			exit(1);
		}
	}
	for (actext = inode->extents; actext; actext = actext->inode_next) {
		for (curpos = 0; curpos < actext->len; curpos += curlen) {
			curlen = min(actext->len - curpos, remap_buf_blocks);
			read_blocks(actext->block + curpos, remap_buf, curlen);
			for (writepos = 0; writepos < curlen; writepos += writelen) {
				freeext = find_remapped_offset(actext->chunk_offset + writepos);
				writelen = min(freeext->len -
					(actext->chunk_offset + writepos - freeext->offset),
					curlen - writepos);
				write_blocks(freeext->block + (actext->chunk_offset + writepos
					- freeext->offset),
					remap_buf + writepos*fs->blocksize, writelen);
			}
		} 
	}
}

int remap_block(ext2_filsys fs, blk_t *blocknr, e2_blkcnt_t offset,
        blk_t parent_blk, int parent_offset, void *priv)
{
	struct block_traverse_data *tdata = priv;
	int depth;

	if (offset < 0) {
		switch (offset) {
			case BLOCK_COUNT_IND:
				if (tdata->depth < 0)
					tdata->depth = 0;
				else
					tdata->tree_ind[0] = parent_offset>>2;
				depth = 0;
				break;
			case BLOCK_COUNT_DIND:
				if (tdata->depth < 1) {
					tdata->depth = 1;
					tdata->tree_ind[0] = 0;
				}
				else
					tdata->tree_ind[1] = parent_offset>>2;
				depth = 1;
				break;
			case BLOCK_COUNT_TIND:
				if (tdata->depth < 2) {
					tdata->depth = 2;
					tdata->tree_ind[0] = tdata->tree_ind[1] = 0;
				}
				depth = 2;
				break;
			default:
				fprintf(stderr, "Unknown block offset %Ld in "
						"blocks scan.\n", (long long)offset);
				exit(1);
		}
		offset = INDIRECT_TO_OFF(index_to_offset(fs, tdata->depth, tdata->tree_ind), depth);
	}
	if (*blocknr >= fs->super->s_blocks_count) {
		fprintf(stderr, "Illegal block %Ld referenced at ino %u offset %Ld\n", (long long)*blocknr, (unsigned)tdata->ino, (long long)offset);
		exit(1);
	}

	if (cmp_ext_offset_ptr_core(offset, tdata->actext->offset + tdata->actext->len - 1) > 0) {
		tdata->actext = tdata->actext->inode_next;
		if (!tdata->actext)
			return BLOCK_ABORT;
	}
	if (offset >= tdata->actext->offset && offset < tdata->actext->offset+
	    tdata->actext->len) {
		struct free_extent *freeext = find_remapped_offset(tdata->actext->chunk_offset +
			offset - tdata->actext->offset);
		if (*blocknr != tdata->actext->block + offset-tdata->actext->offset) {
			fprintf(stderr, "Block changed (%Ld != %Ld) at offset %Ld for inode %u.\n", (long long)*blocknr, (long long)(tdata->actext->block + offset-tdata->actext->offset), (long long)offset, (unsigned)tdata->ino);
			exit(1);
		}
		*blocknr = __cpu_to_le32(freeext->block +
			tdata->actext->chunk_offset + offset - tdata->actext->offset
			 - freeext->offset);
		if (*blocknr >= fs->super->s_blocks_count) {
			fprintf(stderr, "Illegal block %Ld for ino %u computed: freeblock=%Ld, freeoffset=%Ld, offset=%Ld, extoffset=%Ld, chunk_offset=%Ld\n", (long long)*blocknr, (unsigned)tdata->ino, (long long)freeext->block, (long long)freeext->offset, (long long)offset, (long long)tdata->actext->offset, (long long)tdata->actext->chunk_offset);
			exit(1);
		}
		return BLOCK_CHANGED;
	}
	return 0;
}

void update_bitmap(void)
{
	int i, grp;
	struct remap_extent *actext;
	errcode_t error;
	blk_t start, len, end;

	/* Marked remapped blocks as allocated */
	for (i = 0; i < free_extent_count; i++) {
		ext2fs_mark_block_bitmap_range(fs->block_map,
			free_extents[i].block, free_extents[i].len);
		start = free_extents[i].block;
		len = free_extents[i].len;
		while (len) {
			grp = ext2fs_group_of_blk(fs, start);
			end = min(EXT2_BLOCKS_PER_GROUP(fs->super)*(grp+1),
				start+len);
			fs->group_desc[grp].bg_free_blocks_count -= end-start;
			len -= end-start;
			start = end;
		}
	}
	/* Mark original blocks as free */
	for (i = 0; i < inode_count; i++) {
		for (actext = inodes[i].extents; actext; actext = actext->inode_next) {
			ext2fs_unmark_block_bitmap_range(fs->block_map,
				actext->block, actext->len);
			start = actext->block;
			len = actext->len;
			while (len) {
				grp = ext2fs_group_of_blk(fs, start);
				end = min(EXT2_BLOCKS_PER_GROUP(fs->super)*(grp+1),
					start+len);
				fs->group_desc[grp].bg_free_blocks_count += end-start;
				len -= end-start;
				start = end;
			}
		}
	}
	error = ext2fs_write_block_bitmap(fs);
	if (error) {
		fprintf(stderr, "Cannot write block bitmap: %s\n", error_message(error));
		exit(1);
	}
	ext2fs_mark_super_dirty(fs);
}

void copy_and_remap_extents(void)
{
	unsigned int i;
	errcode_t error;
	char *blockbuf = malloc(fs->blocksize * 3);
	struct block_traverse_data tdata;
	struct ext2_inode inode;

	for (i = 0; i < inode_count; i++) {
		error = ext2fs_read_inode(fs, inodes[i].ino, &inode);
		if (error) {
			fprintf(stderr, "Cannot read inode %u: %s\n", (unsigned)inodes[i].ino, error_message(error));
			exit(1);
		}
		if (!inode.i_links_count) {
			fprintf(stderr, "Requested to remap unlinked inode %u.\n", (unsigned)inodes[i].ino);
			exit(1);
		}
		copy_inode_blocks(inodes+i);
		tdata.actext = inodes[i].extents;
		tdata.depth = -1;
		tdata.tree_ind[0] = tdata.tree_ind[1] = 0;
		tdata.ino = inodes[i].ino;
		error = ext2fs_block_iterate2(fs, inodes[i].ino, 0, blockbuf, remap_block, (void *)&tdata);
		if (error) {
			fprintf(stderr, "Failed to iterate over blocks of ino %u: %s\n", (unsigned)inodes[i].ino, error_message(error));
			exit(1);
		}
	}
	free(blockbuf);
	update_bitmap();
}

int main(int argc, char **argv)
{
	errcode_t error;

	initialize_ext2_error_table();
	parse_options(argc, argv);
	fputs("Loading extents to remap...\n", stderr);
	load_extents();
	fputs("Associating extents with inodes...\n", stderr);
	sort_inodes_extents();
	fputs("Searching for free blocks...\n", stderr);
	find_free_blocks();
	fputs("Remapping data into free space...\n", stderr);
	copy_and_remap_extents();

	error = ext2fs_close(fs);
	if (error) {
		fprintf(stderr, "Cannot flush out changes: %s\n", error_message(error));
		return 1;
	}
	fputs("Done.\n", stderr);
	return 0;
}
