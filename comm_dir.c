#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "comm_dir.h"

int comm_dir_rec_init(struct filenode *tdir, unsigned doff) {
	unsigned cur;
	int ret;
	unsigned offset, size, type;
	char name[33];
	struct filenode *node;

	fseek(fs->file, doff, SEEK_SET);
	do {
		memset(name, 0, sizeof(name));
		fread(name, 1, sizeof(name) - 1, fs->file);
		fread(&type, sizeof(unsigned), 1, fs->file);
		fread(&size, sizeof(unsigned), 1, fs->file);
		fread(&offset, sizeof(unsigned), 1, fs->file);
		type &= 0xFF;
		switch (type) {
			case 0:
				node = generic_add_file(tdir, name, FILETYPE_REGULAR);
				if (!node) {
					fprintf(stderr, "Error adding file desciption to library.\n");
					return errno;
				}
				node->offset = offset;
				node->size = size;
				break;
			case 1:
				node = generic_add_file(tdir, name, FILETYPE_DIR);
				if (!node) {
					fprintf(stderr, "Error adding file desciption to library.\n");
					return errno;
				}
				cur = ftell(fs->file);
				ret = comm_dir_rec_init(node, offset);
				if (ret) {
					return ret;
				}
				fseek(fs->file, cur, SEEK_SET);
				break;
			case 0xFF:
				break;
			default:
				fprintf(stderr, "Unexpected file format %i.\n", type);
				return -EINVAL;
		}

	} while (type != 0xFF);

	return 0;
}

int init_game_comm_dir(void) {
	comm_dir_rec_init(fs->root, 0);

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}
