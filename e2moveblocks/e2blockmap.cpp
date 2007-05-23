/*
    Shows map of blocks of files on ext2/3 filesystem.
    (c) 2007 Krzysztof Lichota <krzysiek-ext2tools@lichota.net>
*/

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
    filesystem_info *fs_info
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
        error_msg("Filesystem is mounted\n");
        return 0;
    }

    if (mount_flags & EXT2_MF_BUSY) 
    {
        error_msg("Filesystem is in use\n");
        return 0;
    }
    
    retval = ext2fs_open(
        file_name, 
        0, //flags: read-only
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

    return 1; //KLDEBUG
}

void close_filesystem(filesystem_info *fs_info)
{
    errcode_t retval = ext2fs_close(fs_info->fs);
    if (retval != 0)
    {
        error_msg("Failed to close filesystem, it might be in inconsistent state, error=%s\n", error_message(retval));
    }
}

char const * const ind_string = "IND";
char const * const dind_string = "DIND";
char const * const tind_string = "TIND";
char const * const translator_string = "TIND";
char const * const unknown_string = "Unknown";

char const * block_type_string(e2_blkcnt_t block)
{
    switch (block)
    {
        case BLOCK_COUNT_IND: return ind_string;
        case BLOCK_COUNT_DIND: return dind_string;
        case BLOCK_COUNT_TIND: return tind_string;
        case BLOCK_COUNT_TRANSLATOR: return translator_string;
        default: return unknown_string;
    }
}

inline bool is_metadata_block(e2_blkcnt_t block)
{
    if (block < 0)
    {
        return true;
    }
    return false;
}

int print_file_blocks_verbose(
    ext2_filsys fs, 
    blk_t *blocknr, 
    e2_blkcnt_t blockcnt, 
    blk_t ref_blk, 
    int ref_offset, 
    void *private_data
    )
{
    if (is_metadata_block(blockcnt))
    {
        printf("type=%s diskblock=%u  ref_blk=%u ref_offset=%d\n",
            block_type_string(blockcnt),
            *blocknr,
            ref_blk,
            ref_offset
            );
    }
    else
    {
        printf("blocknumber=%llu diskblock=%u ref_blk=%u ref_offset=%d\n",
            blockcnt,
            *blocknr,
            ref_blk,
            ref_offset
            );
    }
    return 0;
}

typedef int (*print_file_blocks_function_t)(
    ext2_filsys fs, 
    blk_t *blocknr, 
    e2_blkcnt_t blockcnt, 
    blk_t ref_blk, 
    int ref_offset, 
    void *private_data
    );

int show_file_blockmap(
    filesystem_info *fs_info, 
    char const * file_name, 
    print_file_blocks_function_t print_file_blocks_function,
    void *private_data
    )
{
    ext2_ino_t inode_number;
    ext2_ino_t root;
    ext2_ino_t cwd;
    errcode_t retval;
    
    //lookup file name
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
        error_msg("Warning: failed to lookup file, file=%s, error=%s\n", file_name, error_message(retval));
        return 0;
    }
    
    retval = ext2fs_block_iterate2(
        fs_info->fs, 
        inode_number, 
        0, //BLOCK_FLAG_HOLE, //flags
        NULL, //block_buf
        print_file_blocks_function,
        private_data
        );
    
    if (retval != 0) 
    {
        error_msg("Failed to traverse blocks of file, file=%s, error=%s\n", 
            file_name,
            error_message(retval)
            );
        return 0;
    }

    return 1;
}

void usage()
{
    error_msg("Usage: e2blockmap file_name\n");
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        usage();
        exit(-1);
    }

    initialize_ext2_error_table();
    
    filesystem_info fs_info;
    if (!open_filesystem("ext3.img", &fs_info))
    {
        exit(3);
    }
    else
    {
        debug_msg("Successfully opened  filesystem\n");
    }
    
    show_file_blockmap(
        &fs_info,
        argv[1], 
        print_file_blocks_verbose,
        NULL //private_data
        );
    
    close_filesystem(&fs_info);
}
