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

typedef struct {
    ext2_ino_t inode_number;
    e2_blkcnt_t start_offset; //block offset in file from which to start printing, <0 if not used
    e2_blkcnt_t length;  //length in blocks of printed block, <0 to print till end of file
    bool print_holes;
    bool print_indirect;
    void (*print_block_fun)(
        blk_t blocknr, //number of block on disk
        ext2_ino_t inode_number,
        e2_blkcnt_t block_offset, //offset of block in file
        blk_t ref_blk, 
        int ref_offset
        );
    void (*print_indirect_fun)(
        blk_t blocknr, 
        ext2_ino_t inode_number,
        e2_blkcnt_t blockcnt, 
        blk_t ref_blk, 
        int ref_offset
        );
    //private to printing function
    bool in_printing_range;
} print_params_t;

int print_file_blocks(
    ext2_filsys fs, 
    blk_t *blocknr, 
    e2_blkcnt_t blockcnt, 
    blk_t ref_blk, 
    int ref_offset, 
    void *private_data
    )
{
    print_params_t *params = (print_params_t*)private_data;
    
    if (is_metadata_block(blockcnt))
    {
        if (params->print_indirect && params->in_printing_range)
        {
            params->print_indirect_fun(
                *blocknr,
                params->inode_number,
                blockcnt,
                ref_blk,
                ref_offset
                );
        }
    }
    else
    {
        params->in_printing_range = false;
        if (params->start_offset >= 0 && blockcnt < params->start_offset) 
        {
            //not yet in range
            return 0;
        }
        if (params->length >= 0 && blockcnt >= params->start_offset + params->length)
        {
            //went behind range
            return BLOCK_ABORT;
        }
        params->in_printing_range = true;
        params->print_block_fun(
            *blocknr,
            params->inode_number,
            blockcnt,
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
    print_params_t *print_params
    )
{
    ext2_ino_t inode_number;
    ext2_ino_t root;
    ext2_ino_t cwd;
    errcode_t retval;
    unsigned flags = 0;
    
    if (file_name[0] == '/') 
    {
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
    } 
    else if (file_name[0] >= '0' && file_name[0] <= '9')
    {
        //inode number
        inode_number = atol(file_name);
    } 
    else 
    {
            error_msg("Invalid file or inode name %s\n", file_name);
            return 0;
    }
    print_params->inode_number = inode_number;
    
    if (print_params->print_holes == true) 
    {
        flags |= BLOCK_FLAG_HOLE;
    }
    if (print_params->print_indirect == false)
    {
        flags |= BLOCK_FLAG_DATA_ONLY;
    }
    
    retval = ext2fs_block_iterate2(
        fs_info->fs, 
        inode_number, 
        flags,
        NULL, //block_buf
        print_file_blocks_function,
        print_params
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

void print_indirect_verbose(
    blk_t blocknr, 
    ext2_ino_t inode_number,
    e2_blkcnt_t blockcnt, 
    blk_t ref_blk, 
    int ref_offset
    )
{
    printf("diskblock=%u  inode=%u type=%s-indirect_number=%lld ref_blk=%u ref_offset=%d\n",
        blocknr,
        inode_number,
        block_type_string(blockcnt),
        -blockcnt,
        ref_blk,
        ref_offset
        );
}

void print_block_verbose(
    blk_t blocknr, 
    ext2_ino_t inode_number,
    e2_blkcnt_t blockcnt, 
    blk_t ref_blk, 
    int ref_offset
    )
{
    printf("inode=%d blocknumber=%llu diskblock=%u ref_blk=%u ref_offset=%d\n",
        inode_number,
        blockcnt,
        blocknr,
        ref_blk,
        ref_offset
        );
}

void print_indirect(
    blk_t blocknr, 
    ext2_ino_t inode_number,
    e2_blkcnt_t blockcnt, 
    blk_t ref_blk, 
    int ref_offset
    )
{
    printf("%u %u %s-%lld %u %d\n",
        blocknr,
        inode_number,
        block_type_string(blockcnt),
        -blockcnt,
        ref_blk,
        ref_offset
        );
}

void print_block(
    blk_t blocknr, 
    ext2_ino_t inode_number,
    e2_blkcnt_t blockcnt, 
    blk_t ref_blk, 
    int ref_offset
    )
{
    printf("%u %d %lld\n",
        blocknr,
        inode_number,
        blockcnt
        );
}

void usage()
{
    error_msg("Usage: e2blockmap -p params device file_or_inode_name start_offset length\n");
    error_msg("\tparams - print parameters:\n");
    error_msg("\t\tverbose - print descriptions\n");
    error_msg("\t\tindirect - print indirect blocks\n");
    error_msg("\t\tholes - print also holes in file (note that printing holes causes indirect blocks to be printed after real blocks\n");
    error_msg("\tdevice - name of block device or file which contains filesystem\n");
    error_msg("\tfile_or_inode_name - name of file on filesystem (starting with /) or inode number\n");
    error_msg("\tstart_offset - offset in file from which to start printing\n");
    error_msg("\tlength - length of block from start_offset which will be printed, -1 to print till end of file\n");
}

int main(int argc, char **argv)
{
    if (argc != 7)
    {
        usage();
        exit(1);
    }
    
    char *param_token = argv[1];
    char *params_str = argv[2];
    char *devicename = argv[3];
    char *filename = argv[4];
    e2_blkcnt_t start_offset = atol(argv[5]);
    e2_blkcnt_t length = atol(argv[6]);
    
    if (strcmp(param_token, "-p") != 0)
    {
        usage();
        exit(1);
    }
    print_params_t print_params;
    print_params.start_offset = start_offset; //means not used
    print_params.length = length; //means not used
    print_params.print_holes = false;
    print_params.print_indirect = false;
    print_params.print_block_fun = print_block;
    print_params.print_indirect_fun = print_indirect;
    print_params.in_printing_range = false;
    
    char *p = params_str;
    for (;;) 
    {
        char *param = strtok(p, ", ");
        if (param == NULL)
        {
            break;
        }
        p = NULL;
        if (strcmp(param, "verbose") == 0)
        {
            print_params.print_block_fun = print_block_verbose;
            print_params.print_indirect_fun = print_indirect_verbose;
        }
        else if (strcmp(param, "indirect") == 0)
        {
            print_params.print_indirect = true;
        } 
        else if (strcmp(param, "holes") == 0)
        {
            print_params.print_holes = true;
        } 
        else
        {
            error_msg("Unknown print parameter: %s\n", param);
            usage();
            exit(1);
        }
    }
    

    initialize_ext2_error_table();
    
    filesystem_info fs_info;
    if (!open_filesystem(devicename, &fs_info))
    {
        exit(3);
    }
    else
    {
        debug_msg("Successfully opened  filesystem\n");
    }
    
    show_file_blockmap(
        &fs_info,
        filename, 
        print_file_blocks,
        &print_params
        );
    
    close_filesystem(&fs_info);
}
