#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

void make_filename(char *filename, char *name, char *ext) {
	uint32_t i = 0;
	for (uint32_t j = 0; j < 8; j++) {
		if (name[j] == ' ') {
			break;
		}
		filename[i++] = name[j];
	}
	filename[i++] = '.';
	for (uint32_t j = 0; j < 3; j++) {
		if (ext[j] == ' ') {
			break;
		}
		filename[i++] = ext[j];
	}
	filename[i] = 0;
}

int init_arch_afi_fw(void) {
	char magic[3];
	char fname[MAX_PATH];
	char name[8];
	char ext[3];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, sizeof(char), 3, fs->file);
	if (memcmp(magic, "AFI", 3)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}

	fseek(fs->file, 32, SEEK_SET);

	while (true) {
		memset(fname, 0, sizeof(name));
		fread(name, sizeof(char), sizeof(name), fs->file);
		fread(ext, sizeof(char), sizeof(ext), fs->file);

		if (name[0] == 0) {
			break;
		}

		make_filename(fname, name, ext);

		node = generic_add_path(fs->root, fname, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		fseek(fs->file, 5, SEEK_CUR);
		fread(&node->offset, sizeof(uint32_t), 1, fs->file);
		fread(&node->size, sizeof(uint32_t), 1, fs->file);
		fseek(fs->file, 8, SEEK_CUR);

		if (strcmp(fname, "FWIMAGE.FW") == 0) {
			uint32_t fwoffset = node->offset;
			uint32_t saved = ftell(fs->file);
			struct filenode *fwdir = generic_add_path(fs->root, "FWIMAGE_FW", FILETYPE_DIR);
			if (!fwdir) {
				fprintf(stderr, "Error adding file desciption to library.\n");
				return errno;
			}

			fseek(fs->file, fwoffset + 0x200, SEEK_SET);



			while (true) {
				memset(fname, 0, sizeof(name));
				fread(name, sizeof(char), sizeof(name), fs->file);
				fread(ext, sizeof(char), sizeof(ext), fs->file);

				if (name[0] == 0) {
					break;
				}

				make_filename(fname, name, ext);

				node = generic_add_path(fwdir, fname, FILETYPE_REGULAR);
				if (!node) {
					fprintf(stderr, "Error adding file desciption to library.\n");
					return errno;
				}

				fseek(fs->file, 5, SEEK_CUR);
				fread(&node->offset, sizeof(uint32_t), 1, fs->file);
				node->offset = node->offset * 0x200 + fwoffset;
				fread(&node->size, sizeof(uint32_t), 1, fs->file);
				fseek(fs->file, 8, SEEK_CUR);
			}

			fseek(fs->file, saved, SEEK_SET);
		}
	}

	fs->fs_size = generic_subtree_size(fs->root);

	return 0;
}

bool detect_arch_afi_fw(void) {
	char magic[3];

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, sizeof(char), 3, fs->file);
	fseek(fs->file, 0, SEEK_SET);
	return (memcmp(magic, "AFI", 3)) ? false : true;
}
