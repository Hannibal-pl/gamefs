#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "generic.h"

char * strrev(char *str) {
	char tmp;
	int len = strlen(str);
	for (int i = 0; i < (len / 2); i++) {
		tmp = str[len - 1 - i];
		str[len - 1 - i] = str[i];
		str[i] = tmp;
	}

	return str;
}

int init_game_sforce_pak(void) {
	unsigned count;
	unsigned nbase;
	unsigned dbase;
	unsigned offset;
	char magic[21];
	char name[MAX_FILENAME];
	struct filenode *node;

	fseek(fs->file, 4, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "MASSIVE PAKFILE V 4.0", 21)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fseek(fs->file, 76, SEEK_SET);
	fread(&count, sizeof(unsigned), 1, fs->file);
	fseek(fs->file, 4, SEEK_CUR);
	fread(&dbase, sizeof(unsigned), 1, fs->file);
	nbase = 78 + (16 * (count + 1));

	for(int i = 0; i < count; i++) {
		fseek(fs->file, 100 + (i * 16), SEEK_SET);
		fread(&offset, sizeof(unsigned), 1, fs->file);

		memset(name, 0, sizeof(name));
		fseek(fs->file, nbase + (offset & 0xFFFFFF), SEEK_SET);
		fgets(name, sizeof(name), fs->file);
		strrev(name);

		node = generic_add_file(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			if (errno == -EEXIST) { //ignore duplicates
				fseek(fs->file, sizeof(unsigned) * 4, SEEK_CUR);
				continue;
			}
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		fseek(fs->file, 92 + (i * 16), SEEK_SET);
		fread(&node->size, sizeof(unsigned), 1, fs->file);
		fread(&node->offset, sizeof(unsigned), 1, fs->file);
		node->offset += dbase;
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}
