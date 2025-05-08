#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_dk2_wad(void) {
	uint32_t count;
	uint32_t offset;
	uint32_t size;
	uint32_t savedPos;
	char magic[4];
	char name[MAX_PATH];
	char path[MAX_PATH + 1];
	char last_path[MAX_PATH];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "DWFB", 4)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fseek(fs->file, 72, SEEK_SET);
	fread(&count, sizeof(uint32_t), 1, fs->file);
	fseek(fs->file, 12, SEEK_CUR);

	for(uint32_t i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fseek(fs->file, 4, SEEK_CUR);
		fread(&offset, sizeof(uint32_t), 1, fs->file);
		fread(&size, sizeof(uint32_t), 1, fs->file);
		if (size >= MAX_FILENAME) {
			fprintf(stderr, "Filename too long.\n");
			return -ENAMETOOLONG;
		}
		savedPos = ftell(fs->file);
		fseek(fs->file, offset, SEEK_SET);
		fread(name, 1, size, fs->file);
		fseek(fs->file, savedPos, SEEK_SET);

		char *bksl = strrchr(name,'\\');
		if (bksl) {
			memset(last_path, 0 , sizeof(last_path));
			*bksl = 0;
			pathDosToUnix(name, strlen(name));
			strncpy(last_path, name, MAX_PATH);
			*bksl = '/';
			strncpy(path, name, MAX_PATH);
		} else {
			strncpy(path, last_path, MAX_PATH);
			if (strlen(path) > 0) {
				strncat(path, "/", MAX_PATH);
			}
			strncat(path, name, MAX_PATH);
		}

		node = generic_add_path(fs->root, path, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		fread(&node->offset, sizeof(uint32_t), 1, fs->file);
		fread(&node->size, sizeof(uint32_t), 1, fs->file);
		fseek(fs->file, 20, SEEK_CUR);
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

bool detect_game_dk2_wad(void) {
	char magic[4];

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	fseek(fs->file, 0, SEEK_SET);
	return (memcmp(magic, "DWFB", 4)) ? false : true;
}
