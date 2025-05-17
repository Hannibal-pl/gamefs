#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "../gamefs.h"


int init_game_xbf_dat(void) {
	uint32_t dsize;
	uint32_t size;
	uint32_t count;
	char name[MAX_PATH];
	struct filenode *node = NULL, *prev;
	char *filename;
	uint32_t len;
	FILE *fdir;
	char *dir;

	filename = strdup(fs->options.file);
	if (!filename) {
		fprintf(stderr, "Not enough memory for dir filename.\n");
		errno = -ENOMEM;
		return -ENOMEM;
	}
	len = strlen(filename);
	filename[len - 3] = 'C';
	filename[len - 2] = 'A';
	filename[len - 1] = 'T';
	fdir = fopen(filename, "r");
	if (!fdir) {
		filename[len - 3] = 'c';
		filename[len - 2] = 'a';
		filename[len - 1] = 't';
		fdir = fopen(filename, "r");
		free(filename);
		if (!fdir) {
			fprintf(stderr, "Directory file not found.\n");
			errno = -EINVAL;
			return -EINVAL;
		}
	}

	fseek(fdir, 0, SEEK_END);
	dsize = ftell(fdir);
	fseek(fdir, 0, SEEK_SET);

	dir = malloc(dsize);
	if (!dir) {
		fprintf(stderr, "Not enough memory for dir.\n");
		errno = -ENOMEM;
		return -ENOMEM;
	}
	fread(dir, 1, dsize, fdir);
	fclose(fdir);

	uint8_t xor = 0xDB;
	for (uint32_t i = 0; i < dsize; i++, xor++) {
		dir[i] ^= xor;
		if (dir[i] == '\n') { //count lines
			dir[i] = 0;
			count++;
		}
	}

	if (count < 2) {
		free(dir);
		fprintf(stderr, "Directory empty.\n");
		errno = -EINVAL;
		return -EINVAL;
	}

//	if (strcasecmp(basename(fs->options.file), dir)) {
//		free(dir);
//		fprintf(stderr, "Name mismatch.\n");
//		errno = -EINVAL;
//		return -EINVAL;
//	}

	count--; // omit header
	for (uint32_t i = 0, j = 0; i < count; i++) {
		for (; dir[j] != 0; j++); //skip previous
		j++;

		memset(name, 0, sizeof(name));
		sscanf(&dir[j], "%s %u", name, &size);
		pathDosToUnix(name, strlen(name));

		node = generic_add_path(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			free(dir);
			return errno;
		}

		if (i == 0) {
			node->offset = 0;
		} else {
			node->offset = prev->offset + prev->size;
		}
		node->size = size;
		prev = node;
	}

	free(dir);
	fs->fs_size = generic_subtree_size(fs->root);
	generic_xor = 0x33;
	return 0;
}

bool detect_game_xbf_dat(void) {
	return false;
}
