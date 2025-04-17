#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <endian.h>

#include "../gamefs.h"

int init_game_nfs4_viv(void) {
	unsigned count;
	unsigned flen;
	unsigned offset;
	unsigned size;
	char magic[4];
	char name[MAX_FILENAME];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	fread(&flen, sizeof(unsigned), 1, fs->file);
	flen = be32toh(flen);
	if (memcmp(magic, "BIGF", 4)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fseek(fs->file, 0, SEEK_END);
	if (ftell(fs->file) != flen) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}

	fseek(fs->file, 8, SEEK_SET);

	fread(&count, sizeof(unsigned), 1, fs->file);
	count = be32toh(count);
	fseek(fs->file, 4, SEEK_CUR);

	for (int i = 0; i < count; i++) {
		fread(&offset, sizeof(unsigned), 1, fs->file);
		offset = be32toh(offset);
		fread(&size, sizeof(unsigned), 1, fs->file);
		size = be32toh(size);

		memset(name, 0, sizeof(name));
		for (int j = 0; j < sizeof(name); j++) {
			name[j] = fgetc(fs->file);
			if (!name[j]) {
				break;
			}
		}

		if (name[MAX_FILENAME - 1]) {
			fprintf(stderr, "Filename too long.\n");
			return -ENAMETOOLONG;
		}
		node = generic_add_path(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}
		node->size = size;
		node->offset = offset;
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

bool detect_game_nfs4_viv(void) {
	uint32_t flen;
	size_t rlen;
	char magic[4];

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	fread(&flen, sizeof(uint32_t), 1, fs->file);
	fseek(fs->file, 0, SEEK_END);
	rlen = ftell(fs->file);
	fseek(fs->file, 0, SEEK_SET);
	flen = be32toh(flen);
	return ((memcmp(magic, "BIGF", 4) == 0) && (rlen == flen)) ? true : false;
}
