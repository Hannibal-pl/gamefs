#ifndef __GENERIC_H__
#define __GENERIC_H__

#include <stdbool.h>
#include <stdlib.h>

#include "gamefs.h"

#define generic_is_dir(node) generic_is_type((node), FILETYPE_DIR | FILETYPE_ROOT)
#define generic_is_reg(node) generic_is_type((node), FILETYPE_REGULAR | FILETYPE_PACKED)

extern unsigned char generic_xor;

extern bool generic_is_type(struct filenode *node, unsigned type);
extern void generic_deltree(struct filenode *node);
extern unsigned generic_subtree_size(struct filenode *node);
extern void generic_print_tree(struct filenode *node, int level);
extern struct filenode * generic_find_file(struct filenode *node, const char *file);
extern struct filenode * generic_find_path(struct filenode *node, const char *path);
extern struct filenode * generic_add_file(struct filenode *node, const char *file, unsigned flags);
extern struct filenode * generic_add_path(struct filenode *node, const char *path, unsigned flags);
extern struct filenode * generic_add_unknown(unsigned flags);

extern int generic_fo_getattr(const char *path, struct stat *stbuf);
extern int generic_fo_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
extern int generic_fo_open(const char *path, struct fuse_file_info *fi);
extern int generic_fo_open_zlib(const char *path, struct fuse_file_info *fi);
extern int generic_fo_release(const char *path, struct fuse_file_info *fi);
extern int generic_fo_release_mem(const char *path, struct fuse_file_info *fi);
extern int generic_fo_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
extern int generic_fo_read_mem(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
extern int generic_fo_statfs(const char *path, struct statvfs *buf);

extern int generic_initfs(void);
extern void generic_closefs(void);

#endif