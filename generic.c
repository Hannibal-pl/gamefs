#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <zlib.h>

#include "gamefs.h"

struct filesystem filesystem;
struct filesystem *fs = &filesystem;
unsigned char generic_xor = 0;

bool generic_is_type(struct filenode *node, unsigned type) {
	return ((node->flags & FILETYPE_MASK & type) ? true: false);
}

void generic_deltree(struct filenode *node) {
	if (generic_is_dir(node)) {
		for (int i = 0; i < node->used; i++) {
			if (node->children[i]) {
				generic_deltree(node->children[i]);
				node->children[i] = NULL;
			}
		}
		node->used = 0;
		node->alloced = 0;
		free(node->children);
		node->children = NULL;
	} else if (generic_is_reg(node)) {
		if (node->data) {
			free(node->data);
			node->data = NULL;
		}
	}
	free(node);
}

unsigned generic_subtree_size(struct filenode *node) {
	unsigned size ;

	if (generic_is_dir(node)) {
		size = node->size;
		for (int i = 0; i < node->used; i++) {
			if (node->children[i]) {
				size += generic_subtree_size(node->children[i]);
			}
		}
		return size;
	}
	return node->size;
}

void generic_print_tree(struct filenode *node, int level) {
	if (level > 0) {
		for (int i = 0; i < (level - 1); i++) {
			fputs("    ", stderr);
		}
		fputs("\\-- ", stderr);
		fprintf(stderr, "%s\n", node->name);
	} else {
		fprintf(stderr, "/\n");
	}

	if (generic_is_dir(node)) {
		for (int i = 0; i < node->used; i++) {
			if (node->children[i]) {
				generic_print_tree(node->children[i], level + 1);
			}
		}
	}
}

struct filenode * generic_find_file(struct filenode *node, const char *file) {
	if (!node || !file || (strlen(file) >= MAX_FILENAME) || (strlen(file) == 0)) {
		errno = -EINVAL;
		return NULL;
	}
	if (!generic_is_dir(node)) {
		errno = -ENOTDIR;
		return NULL;
	}

	if (strcmp(file, "/") == 0) {
		return node;
	}

	for (int i = 0; i < node->used; i++){
		if (strcmp(node->children[i]->name, file) == 0) {
			return node->children[i];
		}
	}

	errno = -ENOENT;
	return NULL;
}

struct filenode * generic_find_path(struct filenode *node, const char *path) {
	char local_path[MAX_PATH];
	char *path_ptr = NULL;
	struct filenode *local_node = NULL;

	if (!node || !path || (strlen(path) >= MAX_PATH) || (strlen(path) == 0)) {
		errno = -EINVAL;
		return NULL;
	}
	if (!generic_is_dir(node)) {
		errno = -ENOTDIR;
		return NULL;
	}

	if (strcmp(path, "/") == 0) {
		return node;
	}

	if (path[0] == '/') {
		strncpy(local_path, &path[1], MAX_PATH);
	} else {
		strncpy(local_path, path, MAX_PATH);
	}

	path_ptr = strchr(local_path, '/');
	if (path_ptr) {
		*path_ptr = '\0';
		path_ptr++;
		local_node = generic_find_file(node, local_path);
		if (!local_node) {
			errno = -ENOENT;
			return NULL;
		}
		return generic_find_path(local_node, path_ptr);
	}

	return generic_find_file(node, local_path);
}

struct filenode * generic_add_root(void) {
	struct filenode *node;

	node = (struct filenode *)malloc(sizeof(struct filenode));
	if (!node) {
		errno = -ENOMEM;
		return NULL;
	}

	node->flags = FILETYPE_ROOT;
	node->parent = NULL;
	node->size = 0;
	node->links = 2;
	node->alloced = ALLOC_BASE;
	node->used = 0;
	node->children = (struct filenode **)malloc(sizeof(struct filenode *) * ALLOC_BASE);
	if (!node->children) {
		free(node);
		errno = -ENOMEM;
		return NULL;
	}

	return node;
}

struct filenode * generic_add_file(struct filenode *node, const char *file, unsigned flags) {
	struct filenode *new_filenode = NULL;

	if (!node || !file || (strlen(file) >= MAX_FILENAME) || (strlen(file) == 0)) {
		errno = -EINVAL;
		return NULL;
	}
	if (!generic_is_dir(node)) {
		errno = -ENOTDIR;
		return NULL;
	}

	if (generic_find_file(node, file)) {
		errno = -EEXIST;
		return NULL;
	}

	if (node->alloced == (node->used + 1)) {
		struct filenode **new_children = (struct filenode **)realloc(node->children, sizeof(struct filenode *) * (node->alloced + ALLOC_STEP));
		if (!new_children) {
			errno = -ENOMEM;
			return NULL;
		}
		node->children = new_children;
		node->alloced += ALLOC_STEP;
	}

	new_filenode = (struct filenode *)malloc(sizeof(struct filenode));
	if (!new_filenode) {
		errno = -ENOMEM;
		return NULL;
	}

	new_filenode->flags = flags;
	new_filenode->parent = node;
	new_filenode->size = 0;
	new_filenode->atime = filesystem.stat.st_atime;
	new_filenode->mtime = filesystem.stat.st_mtime;
	new_filenode->ctime = filesystem.stat.st_ctime;
	strncpy(new_filenode->name, file, MAX_FILENAME);

	if (generic_is_dir(new_filenode)) {
		new_filenode->links = 2;
		new_filenode->alloced = ALLOC_BASE;
		new_filenode->used = 0;
		new_filenode->children = (struct filenode **)malloc(sizeof(struct filenode *) * ALLOC_BASE);
		if (!new_filenode->children) {
			free(new_filenode);
			errno = -ENOMEM;
			return NULL;
		}
		node->links++;
	} else if (generic_is_reg(new_filenode)) {
		new_filenode->links = 1;
		new_filenode->packed = 0;
		new_filenode->offset = 0;
		new_filenode->open_no = 0;
		new_filenode->data = NULL;
	}

	node->children[node->used++] = new_filenode;
	node->size = node->used * sizeof(struct filenode);

	filesystem.file_no++;
	return new_filenode;
}

struct filenode * generic_add_path(struct filenode *node, const char *path, unsigned flags) {
	char local_path[MAX_PATH];
	char *path_ptr = NULL;
	struct filenode *local_node = NULL;

	if (!node || !path || (strlen(path) >= MAX_PATH) || (strlen(path) == 0) || (strcmp(path, "/") == 0)) {
		errno = -EINVAL;
		return NULL;
	}
	if (!generic_is_dir(node)) {
		errno = -ENOTDIR;
		return NULL;
	}

	if (path[0] == '/') {
		strncpy(local_path, &path[1], MAX_PATH);
	} else {
		strncpy(local_path, path, MAX_PATH);
	}

	path_ptr = strchr(local_path, '/');
	if (path_ptr) {
		*path_ptr = '\0';
		path_ptr++;
		local_node = generic_find_file(node, local_path);
		if (!local_node) {
			local_node = generic_add_file(node, local_path, FILETYPE_DIR);
			if (!local_node) {
				return NULL;
			}
		}
		if (local_node->flags != FILETYPE_DIR) {
			errno = -EEXIST;
			return NULL;
		}
		return generic_add_path(local_node, path_ptr, flags);
	}

	return generic_add_file(node, local_path, flags);
}

struct filenode * generic_add_unknown(unsigned flags) {
	static unsigned nr = 0;
	char path[MAX_PATH];

	if ((flags & FILETYPE_MASK) == FILETYPE_DIR) {
		errno = -EINVAL;
		return NULL;
	}

	snprintf(path, MAX_PATH, "lost+found/file%06u", nr++);
	return generic_add_path(filesystem.root, path, flags);
}

void generic_xor_char(char* buf, int size) {
	if (generic_xor != 0) {
		for (int i = 0; i < size; i++) {
			buf[i] ^= generic_xor;
		}
	}
}

/* FUSE OPERATIONS BEGIN */

int generic_fo_getattr(const char *path, struct stat *stbuf) {
	struct filenode *node = NULL;

	memset(stbuf, 0, sizeof(struct stat));
	node = generic_find_path(filesystem.root, path);
	if (!node) {
		return -ENOENT;
	}
	if (generic_is_dir(node)) {
		stbuf->st_mode = S_IFDIR | 0755;
	} else {
		stbuf->st_mode = S_IFREG | 0644;
	}
	stbuf->st_nlink = node->links;
	stbuf->st_size = (long long)node->size;
	stbuf->st_atime = node->atime;
	stbuf->st_mtime = node->mtime;
	stbuf->st_ctime = node->ctime;
	return 0;
}

int generic_fo_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	struct filenode *node = NULL;

	node = generic_find_path(filesystem.root, path);
	if (!node) {
		return -ENOENT;
	}
	if (!generic_is_dir(node)) {
		return -ENOTDIR;
	}
	node->atime = time(NULL);

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	for (int i = 0; i < node->used; i++) {
		filler(buf, node->children[i]->name, NULL, 0);
	}
	return 0;
}

int generic_fo_open(const char *path, struct fuse_file_info *fi) {
	struct filenode *node = NULL;

	node = generic_find_path(filesystem.root, path);
	if (!node) {
		return -ENOENT;
	}

	if ((fi->flags & 3) != O_RDONLY) {
		return -EACCES;
	}

	if (generic_is_dir(node)) {
		return -EISDIR;
	}

	node->open_no++;
	return 0;
}

int generic_fo_open_zlib(const char *path, struct fuse_file_info *fi) {
	struct filenode *node = NULL;
	void *packed;
	unsigned long length;

	node = generic_find_path(filesystem.root, path);
	if (!node) {
		return -ENOENT;
	}

	if ((fi->flags & 3) != O_RDONLY) {
		return -EACCES;
	}

	if (generic_is_dir(node)) {
		return -EISDIR;
	}

	pthread_mutex_lock(&filesystem.mutex);
	node->data = malloc(node->size);
	packed = malloc(node->packed);
	if (!node->data || !packed) {
		if (node->data) {
			free(node->data);
			node->data = NULL;
		}
		if (packed) {
			free(packed);
		}
		pthread_mutex_unlock(&filesystem.mutex);
		return -ENOMEM;
	}

	length = node->size;
	fseek(filesystem.file, node->offset, SEEK_SET);
	if (generic_is_type(node, FILETYPE_PACKED)) {
		fread(packed, 1, node->packed, filesystem.file);
		uncompress(node->data, &length, packed, node->packed);
	} else {
		fread(node->data, 1, node->size, filesystem.file);
	}

	node->open_no++;
	pthread_mutex_unlock(&filesystem.mutex);
	free(packed);
	return 0;
}

int generic_fo_release(const char *path, struct fuse_file_info *fi) {
	struct filenode *node = NULL;

	node = generic_find_path(filesystem.root, path);
	if (!node) {
		return -ENOENT;
	}

	if (generic_is_dir(node)) {
		return -EISDIR;
	}

	node->open_no--;
	return 0;
}

int generic_fo_release_mem(const char *path, struct fuse_file_info *fi) {
	struct filenode *node = NULL;

	node = generic_find_path(filesystem.root, path);
	if (!node) {
		return -ENOENT;
	}

	if (generic_is_dir(node)) {
		return -EISDIR;
	}

	pthread_mutex_lock(&filesystem.mutex);
	node->open_no--;
	if (node->data && !node->open_no) {
		free(node->data);
		node->data = NULL;
	}
	pthread_mutex_unlock(&filesystem.mutex);
	return 0;
}

int generic_fo_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	struct filenode *node = NULL;

	node = generic_find_path(filesystem.root, path);
	if (!node) {
		return -ENOENT;
	}
	node->atime = time(NULL);

	if (offset < node->size) {
		if (offset + size > node->size) {
			size = node->size - offset;
		}
		pthread_mutex_lock(&filesystem.mutex);
		fseek(filesystem.file, node->offset + offset, SEEK_SET);
		fread(buf, 1, size, filesystem.file);
		generic_xor_char(buf, size);
		pthread_mutex_unlock(&filesystem.mutex);
	} else {
		size = 0;
	}
	return size;
}

int generic_fo_read_mem(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	struct filenode *node = NULL;

	node = generic_find_path(filesystem.root, path);
	if (!node) {
		return -ENOENT;
	}
	node->atime = time(NULL);

	if (offset < node->size) {
		if (offset + size > node->size) {
			size = node->size - offset;
		}
		pthread_mutex_lock(&filesystem.mutex);
		memcpy(buf, node->data + offset, size);
		generic_xor_char(buf, size);
		pthread_mutex_unlock(&filesystem.mutex);
	} else {
		size = 0;
	}
	return size;
}

int generic_fo_statfs(const char *path, struct statvfs *buf) {
	buf->f_namemax = 255;
	buf->f_frsize = buf->f_bsize = 512;
	buf->f_blocks = (filesystem.fs_size + 511) / 512;
	buf->f_bfree = buf->f_bavail = 0;
	buf->f_files = filesystem.file_no;
	buf->f_ffree = 0;
	return 0;
}

/* FUSE OPERATIONS END */

int generic_initfs(void) {
	pthread_mutex_init(&filesystem.mutex, NULL);
	filesystem.fs_size = 0;
	filesystem.file_no = 1; //root
	if (!filesystem.options.file) {
		fprintf(stderr, "You must set game file name.\n");
		errno = -EINVAL;
		return -EINVAL;
	}
	filesystem.file = fopen(filesystem.options.file, "r");
	if (!filesystem.file) {
		fprintf(stderr, "Game file not found.\n");
		errno = -ENOENT;
		return -ENOENT;
	}
	fstat(fileno(filesystem.file), &filesystem.stat);
	filesystem.root = generic_add_root();
	if (!filesystem.root) {
		fprintf(stderr, "Not enough memory to root dir.\n");
		errno = -ENOMEM;
		return -ENOMEM;
	}

	filesystem.operations.getattr = generic_fo_getattr;
	filesystem.operations.readdir = generic_fo_readdir;
	filesystem.operations.open = generic_fo_open;
	filesystem.operations.release = generic_fo_release;
	filesystem.operations.read = generic_fo_read;
	filesystem.operations.statfs = generic_fo_statfs;

	return 0;
}

void generic_closefs(void) {
	if (filesystem.root) {
		generic_deltree(filesystem.root);
	}
	if (filesystem.priv) {
		free(filesystem.priv);
	}
	pthread_mutex_destroy(&filesystem.mutex);
	memset(&filesystem, 0 , sizeof(struct filesystem));
}