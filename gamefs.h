#ifndef __GAMEFS_H__
#define __GAMEFS_H__

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <fuse_opt.h>

#define ALLOC_STEP 16
#define ALLOC_BASE 8

#define MAX_PATH 4096
#define MAX_FILENAME 256
#define MAX_GAMETYPE 32
#define MAX_DESC 256

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
};

extern struct filesystem *fs;

#endif
