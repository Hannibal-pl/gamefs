#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "gamefs.h"

int add_dir(struct filenode *pnode, uint32_t count) {
	uint32_t subcount;
	uint32_t offset;
	uint32_t size;
	uint32_t packed;
	uint32_t type;
	uint64_t afiletime;
	uint64_t mfiletime;
	uint64_t cfiletime;
	char name[MAX_FILENAME];
	struct filenode *node;

	for (int i = 0; i < count; i++) {
		fread(&type, sizeof(uint32_t), 1, fs->file);
		fread(&size, sizeof(uint32_t), 1, fs->file);

		if (size >= MAX_FILENAME) {
			fprintf(stderr, "Filename too long.\n");
			return -ENAMETOOLONG;;
		}
		memset(name, 0, sizeof(name));
		fread(name, 1, size + 1, fs->file);

		type &= 0xFFFFFFFE;
		switch (type) {
			case 0x00020020: //regular
			case 0x00020820: //packed
				fread(&offset, sizeof(uint32_t), 1, fs->file);
				fseek(fs->file, 4, SEEK_CUR);
				fread(&cfiletime, sizeof(uint64_t), 1, fs->file);
				cfiletime -= 116444736000000000ULL;
				fread(&afiletime, sizeof(uint64_t), 1, fs->file);
				afiletime -= 116444736000000000ULL;
				fread(&mfiletime, sizeof(uint64_t), 1, fs->file);
				mfiletime -= 116444736000000000ULL;
				fseek(fs->file, 12, SEEK_CUR);
				fread(&packed, sizeof(uint32_t), 1, fs->file);
				fread(&size, sizeof(uint32_t), 1, fs->file);

				if (offset) {
					if (type == 0x00020020) {
						node = generic_add_file(pnode, name, FILETYPE_REGULAR);
					} else {
						node = generic_add_file(pnode, name, FILETYPE_PACKED);
					}
					if (!node) {
						fprintf(stderr, "Error adding file desciption to library.\n");
							return errno;
					}
					node->offset = offset;
					node->size = size;
					node->packed = packed;
					node->atime = (time_t)(afiletime / 10000000);
					node->mtime = (time_t)(mfiletime / 10000000);
					node->ctime = (time_t)(cfiletime / 10000000);
				}
				break;
			case 0x00020010:
				fread(&cfiletime, sizeof(uint64_t), 1, fs->file);
				cfiletime -= 116444736000000000ULL;
				fread(&afiletime, sizeof(uint64_t), 1, fs->file);
				afiletime -= 116444736000000000ULL;
				fread(&mfiletime, sizeof(uint64_t), 1, fs->file);
				mfiletime -= 116444736000000000ULL;
				fseek(fs->file, 4, SEEK_CUR);
				fread(&subcount, sizeof(uint32_t), 1, fs->file);

				node = generic_add_file(pnode, name, FILETYPE_DIR);
				if (!node) {
					fprintf(stderr, "Error adding file desciption to library.\n");
						return errno;
				}
				node->atime = (time_t)(afiletime / 10000000);
				node->mtime = (time_t)(mfiletime / 10000000);
				node->ctime = (time_t)(cfiletime / 10000000);

				int retval = add_dir(node, subcount);
				if (retval) {
					return retval;
					if (!node) {
						fprintf(stderr, "Error adding file desciption to library.\n");
							return errno;
					}
				}
				break;
			default:
				fprintf(stderr, "Unknown file type 0x%08x\n", type);
				return -EINVAL;
		}
	}
	return 0;
}

int init_game_risen_pak(void) {
	uint32_t count;
	uint32_t offset;
	uint64_t filetime;
	char magic[4];
	int retval;

	fseek(fs->file, 4, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "G3V0", 4)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}

	fseek(fs->file, 32, SEEK_SET);
	fread(&offset, sizeof(uint32_t), 1, fs->file);

	fseek(fs->file, offset + 4, SEEK_SET);

	fread(&filetime, sizeof(uint64_t), 1, fs->file);
	filetime -= 116444736000000000ULL;
	fs->root->ctime = (time_t)(filetime / 10000000);

	fread(&filetime, sizeof(uint64_t), 1, fs->file);
	filetime -= 116444736000000000ULL;
	fs->root->atime = (time_t)(filetime / 10000000);

	fread(&filetime, sizeof(uint64_t), 1, fs->file);
	filetime -= 116444736000000000ULL;
	fs->root->mtime = (time_t)(filetime / 10000000);

	fseek(fs->file, 4, SEEK_CUR);
	fread(&count, sizeof(uint32_t), 1, fs->file);

	retval = add_dir(fs->root, count);
	if (retval != 0) {
		return retval;
	}

	fs->operations.open = generic_fo_open_zlib;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}
