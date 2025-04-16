#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_mm_lod(void) {
	unsigned count;
	unsigned offset;
	char magic[4];
	char name[16];
	struct filenode *node, *pnode;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "LOD\x00", 4)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fseek(fs->file, 256, SEEK_SET);
	memset(name, 0, sizeof(name));
	fread(name, 1, sizeof(name), fs->file);
	pnode = generic_add_file(fs->root, name, FILETYPE_DIR);
	if (!pnode) {
		fprintf(stderr, "Error adding file desciption to library.\n");
		return errno;
	}

	fread(&offset, sizeof(unsigned), 1, fs->file);
	fseek(fs->file, sizeof(unsigned) * 2, SEEK_CUR);
	fread(&count, sizeof(unsigned), 1, fs->file);
	fseek(fs->file, offset, SEEK_SET);

	for(int i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(name, 1, sizeof(name), fs->file);
		node = generic_add_file(pnode, name, FILETYPE_REGULAR);
		if (!node) {
			if (errno == -EEXIST) { //ignore duplicates
				fseek(fs->file, sizeof(unsigned) * 4, SEEK_CUR);
				continue;
			}
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		fread(&node->offset, sizeof(unsigned), 1, fs->file);
		node->offset += 0x120;
		fread(&node->size, sizeof(unsigned), 1, fs->file);
		fseek(fs->file, sizeof(unsigned) * 2, SEEK_CUR);
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}
