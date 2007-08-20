#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <ext2fs/ext2fs.h>
#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2_err.h>
#include <ext2fs/ext2_io.h>

//#define DEBUG_VERBOSE

struct filesystem_info
{
    ext2_filsys fs;
};

void error_msg(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf (stderr, fmt, ap);
    va_end(ap);
}

inline void no_msg(char *fmt, ...)
{
}

#ifndef NDEBUG
#define debug_msg error_msg
#else
#define debug_msg no_msg
#endif //#ifndef NDEBUG

enum
{
    SUPPORTED_EXT_COMPAT_FEATURES =  EXT3_FEATURE_COMPAT_HAS_JOURNAL,
    SUPPORTED_EXT_RO_COMPAT_FEATURES = EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER,
    SUPPORTED_EXT_INCOMPAT_FEATURES = EXT2_FEATURE_INCOMPAT_FILETYPE,
    BITS_PER_BYTE = 8,
};

#define permanent_assert(expression) \
do { \
    if (!(expression)) \
    { \
        error_msg("Failed assertion (%s), function %s, file %s, line %d", #expression, __PRETTY_FUNCTION__, __FILE__, __LINE__ ); \
        abort(); \
    } \
} while (0)

__u64 inode_file_size(ext2_inode *inode)
{
    if (LINUX_S_ISREG(inode->i_mode)) 
    {
        __u64 i_size = (inode->i_size | ((unsigned long long)inode->i_size_high << 32));
        return i_size;
    } 
    else
    {
        __u64 i_size = inode->i_size;
        return i_size;
    }
}

__u64 ceil_div(__u64 number, __u64 divisor)
{
    __u64 result = number / divisor;
    if (number % divisor == 0)
    {
        return result;
    }
    else
    {
        return result + 1;
    }
}

int open_filesystem(
    char *file_name,
    filesystem_info *fs_info,
    bool force
    )
{
    struct ext2_super_block *sb;
    errcode_t   retval = 0;
    int mount_flags;
    
    retval = ext2fs_check_if_mounted(file_name, &mount_flags);
    
    if (retval != 0) 
    {
        error_msg("Cannot check if filesystem is mounted, error=%s\n", error_message(retval));
        return 0;
    }
    
    if (mount_flags & EXT2_MF_MOUNTED) 
    {
        if (force == true)
        {
            error_msg("Warning: forcing changes on mounted filesystem, hope you know what you are doing\n");
        }
        else
        {
            error_msg("Filesystem is mounted\n");
            return 0;
        }
    }

    if (mount_flags & EXT2_MF_BUSY) 
    {
        if (force == true)
        {
            error_msg("Warning: forcing changes on busy filesystem, hope you know what you are doing\n");
        }
        else
        {
            error_msg("Filesystem is in use\n");
            return 0;
        }
    }
    
    retval = ext2fs_open(
        file_name, 
        EXT2_FLAG_RW,
        0, 
        0, 
        unix_io_manager, 
        &fs_info->fs
        );
    
    if (retval != 0)
    {
        error_msg("Failed to open filesystem, error=%s\n", error_message(retval));
        return 0;
    }
    
    sb = fs_info->fs->super;
    
    /*
     * Check for compatibility with the feature sets.  We need to
     * be more stringent than ext2fs_open().
     */
    if (sb->s_feature_compat & ~EXT2_LIB_FEATURE_COMPAT_SUPP)
    {
        error_msg("ext2/3 filesystem has unsupported compat features, filesystem features=0x%x, supported features=0x%x\n",
            sb->s_feature_compat,
            EXT2_LIB_FEATURE_COMPAT_SUPP
            );
        return 0;
    }
    if (sb->s_feature_incompat & ~EXT2_LIB_FEATURE_INCOMPAT_SUPP)
    {
        error_msg("ext2/3 filesystem has unsupported incompat features, filesystem features=0x%x, supported features=0x%x\n",
            sb->s_feature_incompat,
            EXT2_LIB_FEATURE_INCOMPAT_SUPP
            );
        return 0;
    }
    if (sb->s_feature_ro_compat & ~EXT2_LIB_FEATURE_RO_COMPAT_SUPP) {
        error_msg("ext2/3 filesystem has unsupported ro_compat features, filesystem features=0x%x, supported features=0x%x\n",
            sb->s_feature_ro_compat,
            EXT2_LIB_FEATURE_RO_COMPAT_SUPP
            );
        return 0;
    }
#ifdef ENABLE_COMPRESSION
    if (sb->s_feature_incompat & EXT2_FEATURE_INCOMPAT_COMPRESSION) 
    {
        error_msg("Warning: compression support is experimental.\n");
    }
#endif
#ifndef ENABLE_HTREE
    if (sb->s_feature_compat & EXT2_FEATURE_COMPAT_DIR_INDEX) 
    {
        error_msg("Directory indexes not supported\n");
        return 0;
    }
#endif
    retval = ext2fs_read_bitmaps(fs_info->fs);
    
    if (retval != 0)
    {
        error_msg("Failed to read bitmaps, error=%s\n", error_message(retval));
        return 0;
    }

    return 1;
}

void close_filesystem(filesystem_info *fs_info)
{
    errcode_t retval;
    retval = ext2fs_flush(fs_info->fs);
    if (retval != 0)
    {
        error_msg("Failed to flush filesystem, it might be in inconsistent state, error=%s\n", error_message(retval));
    }
    retval = ext2fs_close(fs_info->fs);
    if (retval != 0)
    {
        error_msg("Failed to close filesystem, it might be in inconsistent state, error=%s\n", error_message(retval));
    }
}

/**
    Finds contiguous free space within group which can contain @size_in_blocks blocks.
    If found, returns true and then @found_first_block number of block which begins free space.
*/

int find_contiguous_free_space(
    filesystem_info *fs_info,
    unsigned int size_in_blocks, 
    blk_t *found_first_block
    )
{
    if (fs_info->fs->super->s_free_blocks_count < size_in_blocks)
    {
        debug_msg("Filesystem does not contain enough blocks, free blocks=%d, necessary blocks=%d\n",
            fs_info->fs->super->s_free_blocks_count,
            size_in_blocks
            );
        return 0;
    }
    
    //now look through free blocks bitmap
    unsigned int contiguous_free_blocks = 0;
    for (unsigned int i = fs_info->fs->block_map->start; i < fs_info->fs->super->s_blocks_count; ++i) //why not from 0?
    {
        if (ext2fs_test_block_bitmap(fs_info->fs->block_map, i) == 0)
        {
            debug_msg("0");
            contiguous_free_blocks += 1;
            if (contiguous_free_blocks >= size_in_blocks)
            {
                *found_first_block = i +1 - size_in_blocks;
                debug_msg("Found contiguous free space, start_block=%d, size >= %d\n", 
                    *found_first_block,
                    size_in_blocks
                    );
                return 1;
            }
        }
        else
        {
            debug_msg("1");
            contiguous_free_blocks = 0;
        }
    }
    debug_msg("\n");
    return 0;
}

struct relocation
{
    ext2_ino_t inode_number;
    blk_t block_offset; //offset in file (in blocks)
    blk_t blocks_count; //number of blocks to relocate
};

struct block_range
{
    blk_t start_block;
    blk_t blocks_count;
};

struct block_buffer
{
    char *data; //if NULL, buffer could not be allocated
};

/**
    Returns buffer for one block.
    FIXME: allocate pool for this to avoid allocation.
*/
block_buffer get_block_buffer(
    filesystem_info *fs_info
    )
{
    block_buffer buf;
    buf.data = (char *)malloc(fs_info->fs->blocksize);
    return buf;
}

void free_block_buffer(
    filesystem_info *fs_info,
    block_buffer *buf
    )
{
    if (buf->data != NULL)
    {
        free(buf->data);
    }
    buf->data = NULL;
}

/**
    Relocates block data and updates block bitmaps.
*/
int relocate_raw_block(
    filesystem_info *fs_info,
    blk_t source_block,
    blk_t target_block
    )
{
    errcode_t retval;
    
    //first check bitmaps
    if (ext2fs_test_block_bitmap(fs_info->fs->block_map, source_block) == 0)
    {
        error_msg("Source block is marked as free and should be occupied, source_block=%d\n", source_block);
        return 0;
    }
    if (ext2fs_test_block_bitmap(fs_info->fs->block_map, target_block) != 0)
    {
        error_msg("Target block is marked as occupied and should be free, target_block=%d\n", target_block);
        return 0;
    }
    
    //now move data
    block_buffer block_buf = get_block_buffer(fs_info);
    if (block_buf.data == NULL)
    {
        error_msg("Cannnot allocate space for block in relocate_raw_block()\n");
        return 0;
    }
    
    retval = io_channel_read_blk(
        fs_info->fs->io, 
        source_block,  
        1, 
        block_buf.data
        );
    
    if (retval != 0)
    {
        error_msg("Failed to read source block, block number=%d\n", source_block);
        return 0;
    }
    
    retval = io_channel_write_blk(
        fs_info->fs->io, 
        target_block,  
        1, 
        block_buf.data
        );
    
    if (retval != 0)
    {
        error_msg("Failed to write target block, block number=%d\n", target_block);
        return 0;
    }
    
    free_block_buffer(fs_info, &block_buf);
    
    //now update bitmaps
    ext2fs_mark_block_bitmap(fs_info->fs->block_map, target_block);
    ext2fs_unmark_block_bitmap(fs_info->fs->block_map, source_block);
    ext2fs_mark_bb_dirty(fs_info->fs);
    //and update group descriptors
    fs_info->fs->group_desc[ext2fs_group_of_blk(fs_info->fs, source_block)].bg_free_blocks_count += 1;
    fs_info->fs->group_desc[ext2fs_group_of_blk(fs_info->fs, target_block)].bg_free_blocks_count -= 1;
    return 1;
}

int is_direct_block(blk_t block_in_inode)
{
    if (block_in_inode < EXT2_NDIR_BLOCKS)
    {
        return 1;
    }
    return 0;
}

int is_1indirect_block(filesystem_info *fs_info, blk_t block_in_inode)
{
    blk_t addr_per_block = (blk_t) EXT2_ADDR_PER_BLOCK(fs_info->fs->super);
    
    if (block_in_inode < EXT2_NDIR_BLOCKS)
    {
        return 0;
    }
    block_in_inode -= EXT2_NDIR_BLOCKS;
    if (block_in_inode < addr_per_block)
    {
        return 1;
    }
    return 0;
}

int is_2indirect_block(filesystem_info *fs_info, blk_t block_in_inode)
{
    blk_t addr_per_block = (blk_t) EXT2_ADDR_PER_BLOCK(fs_info->fs->super);
    
    if (block_in_inode < EXT2_NDIR_BLOCKS)
    {
        return 0;
    }
    block_in_inode -= EXT2_NDIR_BLOCKS;
    if (block_in_inode < addr_per_block)
    {
        return 0;
    }
    block_in_inode -= addr_per_block;
    if (block_in_inode < addr_per_block * addr_per_block)
    {
        return 1;
    }
    
    return 0;
}

int is_3indirect_block(filesystem_info *fs_info, blk_t block_in_inode)
{
    blk_t addr_per_block = (blk_t) EXT2_ADDR_PER_BLOCK(fs_info->fs->super);
    
    if (block_in_inode < EXT2_NDIR_BLOCKS)
    {
        return 0;
    }
    block_in_inode -= EXT2_NDIR_BLOCKS;
    if (block_in_inode < addr_per_block)
    {
        return 0;
    }
    block_in_inode -= addr_per_block;
    if (block_in_inode < addr_per_block * addr_per_block)
    {
        return 0;
    }
    //assume we have at most 3-indirect blocks
    return 1;
}

int in_range(blk_t block_number, block_range *range)
{
    if (block_number < range->start_block)
    {
        return 0;
    }
    if (block_number >= range->start_block + range->blocks_count)
    {
        return 0;
    }
    
    return 1;
}

int relocate_block_in_inode(
    filesystem_info *fs_info,
    ext2_ino_t inode_number,
    ext2_inode *inode, 
    int index_in_inode_blocks,
    blk_t *target_block, 
    block_range *relocation_area
    )
{
    blk_t block = inode->i_block[index_in_inode_blocks];
    
    if (block == 0)
    {
        debug_msg("Inode contains hole, so it is not relocated, inode=%d, index_in_inode_blocks=%d\n",
            inode_number,
            index_in_inode_blocks
            );
        return 1;
    }
    
    if (in_range(block, relocation_area))
    {
        debug_msg("Block already in relocation area, skipping, inode=%d, offset=%d, block=%d\n",
            inode_number,
            index_in_inode_blocks,
            block
            );
        return 1;
    }
    
    
    if (! relocate_raw_block(
            fs_info,
            block,
            *target_block
            )
        )
    {
        return 0;
    }
    debug_msg("Successfully relocated block, inode=%d, offset=%d, block=%d\n",
        inode_number,
        index_in_inode_blocks,
        block
        );
    inode->i_block[index_in_inode_blocks] = *target_block;
    *target_block += 1;
    return 1;
}

int relocate_block_in_indirect_block(
    filesystem_info *fs_info,
    ext2_ino_t inode_number,
    blk_t indirect_block_number,
    unsigned int index_in_indirect_block,
    blk_t *target_block, 
    block_range *relocation_area,
    blk_t *next_indirect_block,
    block_buffer *block_buf
    )
{
    int retval;
    permanent_assert(index_in_indirect_block >= 0);
    permanent_assert(index_in_indirect_block < (fs_info->fs->blocksize / sizeof(blk_t)) );
    
    if (indirect_block_number == 0)
    {
        error_msg("Trying to relocate a hole, inode=%d, block=%d, error=%s\n",
            inode_number,
            indirect_block_number,
            error_message(retval)
            );
        return 0;
    }

    retval = ext2fs_read_ind_block(fs_info->fs, indirect_block_number, block_buf->data);
    if (retval != 0)
    {
        error_msg("Cannot read indirect block, inode=%d, block=%d, error=%s\n",
            inode_number,
            indirect_block_number,
            error_message(retval)
            );
        return 0;
    }
    
    blk_t* block_numbers_in_indirect_block =  (blk_t*)block_buf->data;
    blk_t block = block_numbers_in_indirect_block[index_in_indirect_block];
    *next_indirect_block = block;
    
    if (block == 0)
    {
        debug_msg("Indirect block contains hole, so it is not relocated, inode=%d, indirect_block=%d, index_in_indirect_block=%d\n",
            inode_number,
            indirect_block_number,
            index_in_indirect_block
            );
        return 1;
    }
    
    if (in_range(block, relocation_area))
    {
        debug_msg("Block already in relocation area, skipping, inode=%d, block=%d\n",
            inode_number,
            block
            );
        return 1;
    }
    
    if (! relocate_raw_block(
            fs_info,
            block,
            *target_block
            )
        )
    {
        debug_msg("Failed to relocate raw block, inode=%d, block=%d, target_block=%d\n",
            inode_number,
            block,
            *target_block
            );
        return 0;
    }
    block_numbers_in_indirect_block[index_in_indirect_block] = *target_block;
    *next_indirect_block = block_numbers_in_indirect_block[index_in_indirect_block];
    *target_block += 1;
    
    retval = ext2fs_write_ind_block(fs_info->fs, indirect_block_number, block_buf->data);
    if (retval != 0)
    {
        error_msg("Cannot write indirect block, your filesystem might be in inconsistent state, inode=%d, block=%d, error=%s\n",
            inode_number,
            indirect_block_number,
            error_message(retval)
            );
        return 0;
    }
    return 1;
}

int relocate_block(
    filesystem_info *fs_info,
    ext2_ino_t inode_number, 
    blk_t block_in_inode,
    blk_t *target_block,
    block_range *relocation_area
    )
{
    errcode_t retval;
    ext2_inode inode;
    block_buffer block_buf ;
    blk_t addr_per_block = (blk_t) EXT2_ADDR_PER_BLOCK(fs_info->fs->super);
    
    retval = ext2fs_read_inode(
        fs_info->fs,
        inode_number, 
        &inode
        );
        
    if (retval != 0)
    {
        error_msg("Error reading inode %d\n", inode_number);
        goto fail;
    }
    
    if (! ext2fs_inode_has_valid_blocks(&inode))
    {
        debug_msg("Inode %d does not contain blocks, skipping\n", inode_number);
        return 1;
    }
    
    block_buf = get_block_buffer(fs_info);
    if (block_buf.data == NULL)
    {
        error_msg("Cannnot allocate space for block in relocate_raw_block()\n");
        goto fail;
    }
    
    if (is_direct_block(block_in_inode))
    {
        //block to be relocated is in direct blocks
        if (! relocate_block_in_inode(
                fs_info,
                inode_number,
                &inode, 
                block_in_inode,
                target_block, 
                relocation_area
                )
            )
        {
            error_msg("Failed to relocate direct block, inode=%d, block_in_inode=%d\n",
                inode_number,
                block_in_inode
                );
            goto free_buffer_and_fail;
        }
    }
    else if (is_1indirect_block(fs_info, block_in_inode))
    {
        //relocate top indirect block
        if (! relocate_block_in_inode(
                fs_info,
                inode_number,
                &inode, 
                EXT2_IND_BLOCK,
                target_block, 
                relocation_area
                )
            )
        {
            error_msg("Failed to relocate direct block, inode=%d, block_in_inode=%d\n",
                inode_number,
                block_in_inode
                );
            goto free_buffer_and_fail;
        }
        blk_t indirect_block = inode.i_block[EXT2_IND_BLOCK];
        blk_t index_in_indirect_block = block_in_inode - EXT2_NDIR_BLOCKS;
        blk_t dummy_next_indirect_block;
        
        if (indirect_block == 0)
        {
            debug_msg("Hole in 1-level indirect block, skipping, inode=%d, block_in_inode=%d\n",
                inode_number,
                block_in_inode
                );
        }
        else
        {
            if (!relocate_block_in_indirect_block(
                    fs_info,
                    inode_number,
                    indirect_block,
                    index_in_indirect_block,
                    target_block, 
                    relocation_area,
                    &dummy_next_indirect_block,
                    &block_buf
                    )
                )
            {
                error_msg("Failed to relocate block in 1-level indirect block, inode=%d, block_in_inode=%d\n",
                    inode_number,
                    block_in_inode
                    );
                goto free_buffer_and_fail;
            }
        }
    }
    else if (is_2indirect_block(fs_info, block_in_inode))
    {
        //relocate top indirect block
        if (! relocate_block_in_inode(
                fs_info,
                inode_number,
                &inode, 
                EXT2_DIND_BLOCK,
                target_block, 
                relocation_area
                )
            )
        {
            error_msg("Failed to relocate direct block, inode=%d, block_in_inode=%d\n",
                inode_number,
                block_in_inode
                );
            goto free_buffer_and_fail;
        }
        blk_t indirect_block = inode.i_block[EXT2_DIND_BLOCK];
        blk_t index_in_1indirect_block = (block_in_inode - EXT2_NDIR_BLOCKS - addr_per_block) / addr_per_block;
        blk_t index_in_2indirect_block = (block_in_inode - EXT2_NDIR_BLOCKS - addr_per_block) % addr_per_block;
        
        if (indirect_block == 0)
        {
            debug_msg("Hole in 1-level indirect block, skipping, inode=%d, block_in_inode=%d\n",
                inode_number,
                block_in_inode
                );
        }
        else
        {
            blk_t next_indirect_block;
            if (!relocate_block_in_indirect_block(
                    fs_info,
                    inode_number,
                    indirect_block,
                    index_in_1indirect_block,
                    target_block, 
                    relocation_area,
                    &next_indirect_block,
                    &block_buf
                    )
                )
            {
                error_msg("Failed to relocate block in 1-level indirect block, inode=%d, block_in_inode=%d\n",
                    inode_number,
                    block_in_inode
                    );
                goto free_buffer_and_fail;
            }
            if (next_indirect_block == 0)
            {
                debug_msg("Hole in 2-level indirect block, skipping, inode=%d, block_in_inode=%d\n",
                    inode_number,
                    block_in_inode
                    );
            }
            else
            {
                blk_t dummy_next_indirect_block;
                if (!relocate_block_in_indirect_block(
                        fs_info,
                        inode_number,
                        next_indirect_block,
                        index_in_2indirect_block,
                        target_block, 
                        relocation_area,
                        &dummy_next_indirect_block,
                        &block_buf
                        )
                    )
                {
                    error_msg("Failed to relocate block in 2-level indirect block, inode=%d, block_in_inode=%d\n",
                        inode_number,
                        block_in_inode
                        );
                    goto free_buffer_and_fail;
                }
            }
        }
    }
    else
    {
        debug_msg("Relocating 2,3-indirect blocks not implemented yet");
    }
    
    retval = ext2fs_write_inode(
        fs_info->fs, 
        inode_number, 
        &inode
        );
    if (retval != 0)
    {
        error_msg("Error writing inode %d, filesystem might be in inconsistent state, error=%s\n", inode_number, error_message(retval));
        goto free_buffer_and_fail;
    }
    
    free_block_buffer(fs_info, &block_buf);
    
    return 1;
    
free_buffer_and_fail:
    free_block_buffer(fs_info, &block_buf);
fail:
    return 0;
}

/**
    Relocate block from given inode into @target_group and @target_block.
    @relocation_area is the range of blocks where blocks are relocated,
    so blocks in this range should not be relocated again
*/
int relocate_blocks(
    filesystem_info *fs_info,
    relocation *relocation, 
    blk_t *target_block,
    block_range *relocation_area
    )
{
    for (unsigned int i=0; i < relocation->blocks_count; ++i)
    {
        if (! relocate_block(
                fs_info,
                relocation->inode_number, 
                relocation->block_offset + i,
                target_block, 
                relocation_area
                )
            )
        {
            error_msg("Failed to relocate block %d in relocation of inode %d, block_offset=%d, blocks_count=%d, target_block after relocation=%d\n",
                i,
                relocation->inode_number,
                relocation->block_offset,
                relocation->blocks_count,
                *target_block
                );
            return 0;
        }
    }
    return 1;
}

/**
    Estimates number of blocks (including indirect blocks) occupied by block range in  file, assuming it does not contain holes.
    
    @param start_block First block of file taken into account.
    @param count Number of blocks from start_block occupied in file.
*/
e2_blkcnt_t  estimate_blocks_for_range(
    filesystem_info *fs_info,
    blk_t start_block,
    blk_t count
    )
{
    blk_t addr_per_block = (blk_t) EXT2_ADDR_PER_BLOCK(fs_info->fs->super);
    blk_t end_block = start_block + count - 1;
    permanent_assert(count >= 0);
    
    if (count == 0)
    {
        return 0;
    }
    
    e2_blkcnt_t blocks_count = 0;
    
    if (is_direct_block(start_block))
    {
        blk_t last_direct_block = EXT2_NDIR_BLOCKS - 1;
        if (is_direct_block(end_block))
        {
            blocks_count += end_block - start_block + 1;
        }
        else 
        {
            blocks_count += last_direct_block - start_block + 1;
            start_block = EXT2_NDIR_BLOCKS;
        }
    }
    
    if (is_1indirect_block(fs_info, start_block))
    {
        blk_t last_1indirect_block = EXT2_NDIR_BLOCKS + addr_per_block - 1;
        
        if (is_1indirect_block(fs_info, end_block))
        {
            blocks_count += end_block - start_block + 1;
            blocks_count += 1; //1 indirect metadata block
        }
        else 
        {
            blocks_count += last_1indirect_block - start_block + 1; //data blocks 
            blocks_count += 1; //1 indirect metadata block
            start_block = last_1indirect_block + 1;
        }
    }
    
    if (is_2indirect_block(fs_info, start_block))
    {
        blk_t first_2_indirect_block = EXT2_NDIR_BLOCKS + addr_per_block;
        blk_t last_2indirect_block = EXT2_NDIR_BLOCKS + addr_per_block + addr_per_block*addr_per_block - 1;
        
        if (is_2indirect_block(fs_info, end_block))
        {
            blocks_count += end_block - start_block + 1; //data blocks
            blocks_count += 1; //2 indirect metadata block
            blocks_count += ((end_block - first_2_indirect_block)/addr_per_block) - ((start_block-first_2_indirect_block)/addr_per_block) + 1; //1-indirect blocks
        }
        else 
        {
            blocks_count += last_2indirect_block - start_block + 1; //data blocks 
            blocks_count += 1; //2 indirect metadata block
            blocks_count += (last_2indirect_block/addr_per_block) - (start_block/addr_per_block) + 1; //1-indirect blocks
            start_block = last_2indirect_block + 1;
        }
    }
    
    if (is_3indirect_block(fs_info, start_block))
    {
        blk_t first_3indirect_block = EXT2_NDIR_BLOCKS + addr_per_block + addr_per_block*addr_per_block;
        
        if (is_3indirect_block(fs_info, end_block))
        {
            //FIXME: correct?
            blocks_count += end_block - start_block + 1; //data blocks
            blocks_count += ((end_block - first_3indirect_block)/addr_per_block) - ((start_block-first_3indirect_block)/addr_per_block) + 1; //2-indirect blocks
            blocks_count += ((end_block-first_3indirect_block)/(addr_per_block*addr_per_block)) - ((start_block-first_3indirect_block)/(addr_per_block*addr_per_block)) + 1; //2-indirect blocks
            blocks_count += 1; //3 indirect metadata block
        }
        else 
        {
            permanent_assert(false); //invalid end block number
        }
    }
    
    return blocks_count;
}

void test_estimate_blocks_for_range(filesystem_info *fs_info)
{
    blk_t addr_per_block = (blk_t) EXT2_ADDR_PER_BLOCK(fs_info->fs->super);

    permanent_assert(estimate_blocks_for_range(fs_info, 0, 0) == 0);
    permanent_assert(estimate_blocks_for_range(fs_info, 0, 1) == 1);
    permanent_assert(estimate_blocks_for_range(fs_info, 1, 1) == 1);
    permanent_assert(estimate_blocks_for_range(fs_info, 4, 5) == 5);
    permanent_assert(estimate_blocks_for_range(fs_info, EXT2_NDIR_BLOCKS, 1) == 2); //1 indirect block
    permanent_assert(estimate_blocks_for_range(fs_info, EXT2_NDIR_BLOCKS - 2, 6) == 7);//1 indirect block, spanning direct and indirect blocks
    permanent_assert(estimate_blocks_for_range(fs_info, EXT2_NDIR_BLOCKS + 2, 6) == 7);//1 indirect block, all inside 1-indirect block
    permanent_assert(estimate_blocks_for_range(fs_info, EXT2_NDIR_BLOCKS + addr_per_block - 1, 1) == 2);//1 indirect block, all inside 1-indirect block
    
    permanent_assert(estimate_blocks_for_range(fs_info, EXT2_NDIR_BLOCKS + addr_per_block , 1) == 3);//2-indirect block and 1-indirect block, all inside 2-indirect block
    permanent_assert(estimate_blocks_for_range(fs_info, EXT2_NDIR_BLOCKS + addr_per_block - 2, 6) == 9);//spanning from 1-indirect to 2-indirect: 1 1-indirect block, 1 2-indirect block and 1 1-indirect block
permanent_assert(estimate_blocks_for_range(fs_info, EXT2_NDIR_BLOCKS + addr_per_block + 2, 6) == 8);//1 2-indirect block and 1 1-indirect block
    permanent_assert(estimate_blocks_for_range(fs_info, EXT2_NDIR_BLOCKS + addr_per_block + addr_per_block - 3, 10) == 10 + 1 + 2);//1 2-indirect block and 2 1-indirect block
    permanent_assert(estimate_blocks_for_range(fs_info, EXT2_NDIR_BLOCKS + addr_per_block, addr_per_block*addr_per_block) == addr_per_block*addr_per_block + 1 + addr_per_block);//1 2-indirect block and addr_per_block 1-indirect blocks
        //FIXME: more tests
    permanent_assert(estimate_blocks_for_range(fs_info, EXT2_NDIR_BLOCKS + addr_per_block + addr_per_block*addr_per_block - 1, 1) == 3);//all in 2-indirect area: 1 2-indirect block and 1 1-indirect block
    permanent_assert(estimate_blocks_for_range(fs_info, EXT2_NDIR_BLOCKS + addr_per_block + addr_per_block*addr_per_block, 1) == 4);//all in 3-indirect area: 1 3-indirect block, 1 2-indirect block and 1 1-indirect block
    permanent_assert(estimate_blocks_for_range(fs_info, EXT2_NDIR_BLOCKS + addr_per_block + addr_per_block*addr_per_block - 3, 10) == 15);//span 2- and 3-indirect area: 1 2-indirect, 1 1-indirect plus 1 3-indirect block, 1 2-indirect block and 1 1-indirect block
    permanent_assert(estimate_blocks_for_range(fs_info, EXT2_NDIR_BLOCKS + addr_per_block + addr_per_block*addr_per_block, addr_per_block*addr_per_block*addr_per_block) == addr_per_block*addr_per_block*addr_per_block + 1 + addr_per_block + addr_per_block*addr_per_block);//span all blocks in 3-indirect area
    permanent_assert(estimate_blocks_for_range(fs_info, EXT2_NDIR_BLOCKS + addr_per_block + addr_per_block*addr_per_block + 1, addr_per_block) == addr_per_block + 1 + 1 + 2);//span 2 1-indirect blocks in 3-indirect area: 1 3-indirect block, 1 2-indirect block and 2 1-indirect blocks
    permanent_assert(estimate_blocks_for_range(fs_info, EXT2_NDIR_BLOCKS + addr_per_block + addr_per_block*addr_per_block + addr_per_block*5, addr_per_block*addr_per_block) == addr_per_block*addr_per_block + 1 + 2 + addr_per_block);//span 2 2-indirect blocks in 3-indirect area: 1 3-indirect block, 2 2-indirect block and addr_per_block 1-indirect blocks
//    __u64 maxInt = (1LL << 32) - 1;
    //4 GB file with 1K blocks
//    permanent_assert(estimate_blocks_for_range(fs_info, 0, maxInt/1024) == 4*1024*1024 + 16*1024 + 64 + 1);
    debug_msg("Tests OK\n");
}

int process_layout_list(
    filesystem_info *fs_info,
    char * file_name,
    e2_blkcnt_t *estimated_block_size,
    relocation **relocations,
    int *num_relocations
    )
{
    enum
    {
        BUF_SIZE = 1024,
    };
    char line_buf[BUF_SIZE];
    FILE *list_file = fopen(file_name, "r");
    
    if (list_file == NULL)
    {
        perror("Error opening file\n");
        error_msg("Cannot open layout file %s\n", file_name);
        return 0;
    }
    
    int memory_for_relocations = 20000;
    (*relocations) = (relocation *)malloc(memory_for_relocations*sizeof(relocation));
    if ((*relocations) == NULL)
    {
        error_msg("Cannot allocate memory for relocations structure\n");
        return 0;
    }
    *num_relocations = 0;
    *estimated_block_size = 0;
    
    int page_size = sysconf(_SC_PAGESIZE);
    int block_size = fs_info->fs->blocksize;
    int page_to_blocks_factor = page_size / block_size;
    if (page_to_blocks_factor * block_size != page_size)
    {
        error_msg("Unsupported configuration: page size is not multiple of block size, page size=%d, block size=%d\n", page_size, block_size);
        return 0;
    }
    
    for (;;)
    {
        if (fgets(line_buf, BUF_SIZE, list_file) == NULL)
        {
            break;
        }
        
        if (line_buf[0] == '#')
        {
            //skip comments
            continue;
        }
        if (line_buf[0] == '\n' || line_buf[0] == '\0')
        {
            //skip empty lines
            continue;
        }
        
        char *delimeters = " \t\n";
        char *field1 = strtok(line_buf, delimeters);
        if (field1 == NULL)
        {
            error_msg("Ignoring invalid layout line: %s\n", line_buf);
            continue;
        }
        
        char *field2 = strtok(NULL, delimeters);
        char *field3 = strtok(NULL, delimeters);
        debug_msg("field1=%s, field2=%s, field3=%s\n", field1 ? field1 : "None", field2 ? field2 : "None", field3 ? field3 : "None");
        
        
        char *file_name = field1;
        char *offset_field = field2;
        char *length_field = field3;
        ext2_ino_t inode_number;
        ext2_inode inode;
        ext2_ino_t root;
        ext2_ino_t cwd;
        int retval;
        
        if (file_name[0] == '/') 
        {
            //file name - lookup it up
            root = cwd = EXT2_ROOT_INO;
            retval = ext2fs_namei_follow(
                fs_info->fs, 
                root, 
                cwd, 
                file_name,
                &inode_number
                );
            if (retval != 0)
            {
                error_msg("Warning: failed to lookup file %s, skipping it, error=%s\n", file_name, error_message(retval));
                continue;
            }
        } 
        else if (file_name[0] >= '0' && file_name[0] <= '9')
        {
            //inode number
            inode_number = atol(file_name);
        } 
        else 
        {
                error_msg("Warning: invalid file or inode name %s, skipping it\n", file_name);
                continue;
        }
        
        if (inode_number < EXT2_FIRST_INO(fs_info->fs->super))
        {
                error_msg("Warning: trying to remap system inode %d (entry name %s), skipping it\n", inode_number, file_name);
                continue;
        }
        
        if ((*num_relocations) >= memory_for_relocations)
        {
            //FIXME: realloc?
            error_msg("Too many relocations, relocations limit=%d\n", memory_for_relocations);
            return 1;
        }
        retval = ext2fs_read_inode(
            fs_info->fs,
            inode_number, 
            &inode
            );
            
        if (retval != 0)
        {
            error_msg("Error reading inode %d (entry name %s), skipping it\n", inode_number, file_name);
            continue;
        }
        
        (*relocations)[*num_relocations].inode_number = inode_number;
        unsigned file_size_in_blocks = ceil_div(inode_file_size(&inode), fs_info->fs->blocksize);
        
        if (offset_field == NULL)
        {
            (*relocations)[*num_relocations].block_offset = 0;
        } 
        else 
        {
            //input fields are in PAGE_SIZE units, scale it to blocks
            (*relocations)[*num_relocations].block_offset = atol(offset_field) * page_to_blocks_factor;
        }
        
        if ((*relocations)[*num_relocations].block_offset >= file_size_in_blocks)
        {
                //block begins after file end, skip it
                debug_msg("Relocation block starts after file end, inode %d (entry name %s), start=%d, file size in blocks=%u, skipping it\n", 
                    inode_number, 
                    file_name, 
                    (*relocations)[*num_relocations].block_offset,
                    file_size_in_blocks
                    );
                continue;
        }
        
        if (length_field == NULL) 
        {
            //fill the rest until file end
            (*relocations)[*num_relocations].blocks_count = file_size_in_blocks - (*relocations)[*num_relocations].block_offset;
        } 
        else
        {
            //input fields are in PAGE_SIZE units, scale it to blocks
            (*relocations)[*num_relocations].blocks_count = atol(length_field) * page_to_blocks_factor;
        }
        if ((*relocations)[*num_relocations].block_offset + (*relocations)[*num_relocations].blocks_count > file_size_in_blocks)
        {
            //block end exceeds file end, cut it down to file size
            unsigned new_count = file_size_in_blocks - (*relocations)[*num_relocations].block_offset;
            debug_msg("Relocation block length exceeds file end, inode %d (entry name %s), start=%d, length=%d, file size in blocks=%u, cut it down to %d\n", 
                inode_number, 
                file_name, 
                (*relocations)[*num_relocations].block_offset,
                (*relocations)[*num_relocations].blocks_count,
                file_size_in_blocks,
                new_count
                );
            (*relocations)[*num_relocations].blocks_count = new_count;
        }
        e2_blkcnt_t estimated_blocks_count = estimate_blocks_for_range(
            fs_info,
            (*relocations)[*num_relocations].block_offset,
            (*relocations)[*num_relocations].blocks_count
            );
        (*estimated_block_size) += estimated_blocks_count;
        debug_msg("File %s, inode %d, file size=%llu, start_block=%d, blocks_count=%d, estimated_blocks_count=%d\n",
            file_name,
            inode_number,
            inode_file_size(&inode),
            (*relocations)[*num_relocations].block_offset,
            (*relocations)[*num_relocations].blocks_count,
            estimated_blocks_count
            );
        ++(*num_relocations);
    }
    
    fclose(list_file);
    return 1;
}

void usage()
{
    fprintf(stderr, "Usage: e2moveblocks device layout-file\n");
    fprintf(stderr, "\tdevice - name of block device or file name containing filesystem\n");
    fprintf(stderr, "\tlayout-file - file with requested layout of files in filesystem\n");
}

int main(int argc, char **argv)
{
    char *devicename = argv[1];
    char *layoutfile = argv[2];
    bool force = false;
    
    if (argc < 3 || argc > 4)
    {
        usage();
        exit(1);
    }
    if (argc == 4)
    {
        if (strcmp(argv[1], "--force") == 0)
        {
            force = true;
            devicename = argv[2];
            layoutfile = argv[3];
        }
        else
        {
            usage();
            exit(1);
        }
    }
    

    initialize_ext2_error_table();
    
    filesystem_info fs_info;
    if (!open_filesystem(devicename, &fs_info, force))
    {
        exit(3);
    }
    else
    {
        debug_msg("Successfully opened  filesystem\n");
    }
    
    test_estimate_blocks_for_range(&fs_info);
    
    e2_blkcnt_t size_in_blocks;
    
    relocation *relocations;
    int num_relocations;

    if (! process_layout_list(&fs_info, layoutfile, &size_in_blocks, &relocations, &num_relocations))
    {
        error_msg("Cannot process relocations list\n");
        exit(5);
    }
#ifdef DEBUG_VERBOSE
    debug_msg("Relocations:\n");
    for (int i = 0; i < num_relocations; ++i) 
    {
        debug_msg("Reloc: inode=%d start=%d count=%d\n", 
            relocations[i].inode_number,
            relocations[i].block_offset,
            relocations[i].blocks_count
            );
        
    }
#endif

    
    blk_t found_first_block;
    if (! find_contiguous_free_space(&fs_info, size_in_blocks, &found_first_block))
    {
        error_msg("Cannot find group with %d contiguous free blocks\n", size_in_blocks);
        exit(4);
    }
    
    block_range relocation_area = 
    {
        found_first_block,
        size_in_blocks,
    };
    
    blk_t relocation_block = found_first_block;
    
    for (int i=0; i < num_relocations; ++i)
    {
        if (!relocate_blocks(
                &fs_info,
                &relocations[i], 
                &relocation_block, 
                &relocation_area
                )
            )
        {
            error_msg("Failed to relocate inode %d, block_offset=%d, blocks_count=%d\n",
                relocations[i].inode_number,
                relocations[i].block_offset,
                relocations[i].blocks_count
                );
            exit(10);
        }
    }
    
    close_filesystem(&fs_info);
}
