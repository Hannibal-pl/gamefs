#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "generic.h"

struct dir {
	struct filenode *dir;
	unsigned nr;
};

int init_game_toee_dat(void) {
	unsigned offset;
	unsigned count;
	unsigned fnsize;
	unsigned tmp, size, packed, foff, pdir, cdir;
	unsigned char magic[4];
	char name[MAX_FILENAME];
	struct filenode *node, *root;
	struct dir *dir_list, *new_list;
	unsigned dir_list_size = 1;

	fseek(fs->file, -12, SEEK_END);
	fread(magic, sizeof(char), 4, fs->file);
	if (memcmp(magic, "1TAD", 4)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fseek(fs->file, -4, SEEK_END);
	fread(&offset, sizeof(unsigned), 1, fs->file);
	fseek(fs->file, -offset, SEEK_END);
	fread(&count, sizeof(unsigned), 1, fs->file);

	dir_list = (struct dir *)malloc(sizeof(struct dir));
	if (!dir_list) {
		fprintf(stderr, "Not enough memory to temporary directory.\n");
		return -ENOMEM;
	}
	dir_list[0].dir = fs->root;
	dir_list[0].nr = 0xFFFFFFFF;

	for(int i = 0; i < count; i++) {
		memset(name, 0 ,sizeof(name));
		fread(&fnsize, sizeof(unsigned), 1, fs->file);
		if (fnsize >= MAX_FILENAME) {
			fprintf(stderr, "Filename too long.\n");
			return -ENAMETOOLONG;
		}
		fread(name, fnsize, 1, fs->file);
		fseek(fs->file, 4, SEEK_CUR);
		fread(&tmp, sizeof(unsigned), 1, fs->file);
		fread(&size, sizeof(unsigned), 1, fs->file);
		fread(&packed, sizeof(unsigned), 1, fs->file);
		fread(&foff, sizeof(unsigned), 1, fs->file);
		fread(&pdir, sizeof(unsigned), 1, fs->file);
		fread(&cdir, sizeof(unsigned), 1, fs->file);
		fseek(fs->file, 4, SEEK_CUR);

		root = fs->root;
		for(int j = 0; j < dir_list_size; j++) {
			if (dir_list[j].nr == pdir) {
				root = dir_list[j].dir;
				break;
			}
		}

		if (tmp == 2) {
			node = generic_add_file(root, name, FILETYPE_PACKED);
		} else if (tmp == 1) {
			node = generic_add_file(root, name, FILETYPE_REGULAR);
		} else {
			node = generic_add_file(root, name, FILETYPE_DIR);
		}
		if (!node) {
			if (errno == -EEXIST) { //ignore duplicates
				continue;
			}
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		if (generic_is_dir(node)) {
			new_list = (struct dir *)realloc(dir_list, sizeof(struct dir) * (dir_list_size + 1));
			if (!new_list) {
				free(dir_list);
				fprintf(stderr, "Not enough memory to temporary directory.\n");
				return -ENOMEM;
			}
			dir_list = new_list;

			dir_list[dir_list_size].dir = node;
			dir_list[dir_list_size++].nr = cdir - 1;
		} else {
			node->size = size;
			node->packed = packed;
			node->offset = foff;
		}
	}

	if (dir_list) {
		free(dir_list);
	}

	fs->operations.open = generic_fo_open_zlib;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;

	fs->fs_size = generic_subtree_size(fs->root);

	return 0;
}
