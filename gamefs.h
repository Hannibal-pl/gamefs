#ifndef __GAMEFS_H__
#define __GAMEFS_H__

#define FUSE_USE_VERSION 26

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <fuse.h>
#include <fuse_opt.h>

#define ALLOC_STEP 16
#define ALLOC_BASE 8

#define MAX_PATH 4096
#define MAX_FILENAME 256
#define MAX_GAMETYPE 32
#define MAX_DESC 256

#define INFLATE_CHUNK 4096

enum {
	FILETYPE_EMPTY = 0,
	FILETYPE_REGULAR = 1,
	FILETYPE_PACKED = 2,
	FILETYPE_DIR = 4,
	FILETYPE_ROOT = 8,
	FILETYPE_MASK = 0xFF,
};

struct options {
	char* game;
	char* file;
	char* param;
};

struct filenode;

struct filesystem {
	FILE *file;
	pthread_mutex_t mutex;
	unsigned fs_size;
	unsigned file_no;
	struct stat stat;
	struct options options;
	struct fuse_operations operations;
	struct filenode *root;
	void *priv;
};

struct filenode {
	char name[MAX_FILENAME];
	unsigned size;
	unsigned links;
	unsigned flags;
	struct filenode *parent;
	time_t atime;
	time_t mtime;
	time_t ctime;
	union {
		struct {
			unsigned alloced;
			unsigned used;
			struct filenode **children;
		}; //dir
		struct {
			unsigned packed;
			union {
				unsigned offset;
				unsigned cluster;
			};
			unsigned open_no;
			unsigned local_flags;
			void *data;
		}; //file
	};
};

struct gametable {
	char game[MAX_GAMETYPE];
	char description[MAX_DESC];
	int (*initgame)(void);
	bool (*autodetect)(void);
};

extern struct filesystem *fs;

//generic
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

//tools
extern void pathDosToUnix(char *path, uint32_t len);
extern void strrev(char *str, uint32_t len);
extern bool unpackSizeless(uint8_t *in, uint32_t insize, uint8_t **out, uint32_t *outsize);
extern time_t fatTime(uint32_t val);

#endif