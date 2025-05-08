#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

void addDir(struct filenode* parent, uint32_t doffset, uint32_t dsize) {
	uint32_t i = 0;
	uint32_t type;
	uint32_t offset;
	uint32_t size;
	uint32_t fattime;
	uint32_t fpos;
	char name[MAX_FILENAME + 1];
	char ext[5] = ".";
	struct filenode *node;

	fseek(fs->file, doffset, SEEK_SET);
	while (i < dsize) {
		memset(name, 0, sizeof(name));
		fread(&type, sizeof(uint32_t), 1, fs->file); i += 4;
		fread(&offset, sizeof(uint32_t), 1, fs->file); i += 4;
		fread(&size, sizeof(uint32_t), 1, fs->file); i += 4;
		fread(&fattime, sizeof(uint32_t), 1, fs->file); i += 4;

		if (type == 1) { //dir
			for (uint32_t j = 0; j < MAX_FILENAME; j++) {
				fread(&name[j], 1, 1, fs->file); i += 1;
				if (name[j] == 0) {
					break;
				}
			}
			node = generic_add_file(parent, name, FILETYPE_DIR);
			node->atime = node->mtime = node->ctime = fatTime(fattime);

			fpos = ftell(fs->file);
			addDir(node, offset, size);
			fseek(fs->file, fpos, SEEK_SET);
		} else { //file
			fseek(fs->file, 4, SEEK_CUR); i += 4;
			fread(&ext[3], 1, 1, fs->file); i += 1;
			fread(&ext[2], 1, 1, fs->file); i += 1;
			fread(&ext[1], 1, 1, fs->file); i += 1;
			fseek(fs->file, 5, SEEK_CUR); i += 5;

			for (uint32_t j = 0; j < MAX_FILENAME - 4; j++) {
				fread(&name[j], 1, 1, fs->file); i += 1;
				if (name[j] == 0) {
					fseek(fs->file, 1, SEEK_CUR); i += 1;
					break;
				}
			}

			if (strlen(ext) > 1) { //don't add alone dot
				strncat(name, ext, MAX_FILENAME);
			}

			node = generic_add_file(parent, name, FILETYPE_REGULAR);
			node->offset = offset;
			node->size = size;
			node->atime = node->mtime = node->ctime = fatTime(fattime);

		}
	}

}

int init_game_nolf_rez(void) {
	uint32_t offset;
	uint32_t size;
	uint32_t fattime;
	char magic[126];

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic,	"\r\nRezMgr Version 1 Copyright (C) 1995 MONOLITH INC.           "
				"\r\nLithTech Resource File                                      \r\n", 126)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fseek(fs->file, 131, SEEK_SET);
	fread(&offset, sizeof(uint32_t), 1, fs->file);
	fread(&size, sizeof(uint32_t), 1, fs->file);
	fread(&fattime, sizeof(uint32_t), 1, fs->file);
	fs->root->atime = fs->root->mtime = fs->root->ctime = fatTime(fattime);

	addDir(fs->root, offset, size);
	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

bool detect_game_nolf_rez(void) {
	char magic[126];

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	fseek(fs->file, 0, SEEK_SET);
	return (memcmp(magic,	"\r\nRezMgr Version 1 Copyright (C) 1995 MONOLITH INC.           "
				"\r\nLithTech Resource File                                      \r\n", 126)) ? false : true;
}
