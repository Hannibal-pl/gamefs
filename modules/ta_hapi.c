#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <zlib.h>

#include "../gamefs.h"

struct private {
	bool is_encrypted;
	unsigned key;
};

struct dir_head {
	unsigned entries;
	unsigned offset;
};

struct __attribute__ ((__packed__)) dir_entry {
	unsigned name;
	unsigned offset;
	char flags;
};

struct __attribute__ ((__packed__)) file_entry {
	unsigned offset;
	unsigned size;
	char flags;
};

struct __attribute__ ((__packed__)) chunk_header {
	unsigned char magic[4];
	char reserved;
	char compression;
	char is_encrypt;
	unsigned packed;
	unsigned unpacked;
	long checksum;
};


int ta_hapi_read(unsigned char *buf, unsigned size, FILE *file) {
	struct private *priv = (struct private *)fs->priv;
	unsigned pos = ftell(file);
	unsigned realread;

	realread = fread(buf, sizeof(unsigned char), size, file);

	if (priv->is_encrypted) {
		for (int i = 0; i < realread; i++, pos++) {
			buf[i] = ~buf[i] ^ pos ^ priv->key;
		}
	}

	return realread;
}

char * ta_hapi_fgets0(char *s, unsigned size, FILE *file) {
	struct private *priv = (struct private *)fs->priv;
	unsigned pos = ftell(file);
	int i;

	for (i = 0; i < (size - 1); i++) {
		s[i] = fgetc(file);
		if (priv->is_encrypted) {
			s[i] = ~s[i] ^ pos++ ^ priv->key;
		}
		if (s[i] == 0) {
			break;
		}
	}
	s[i] = 0;

	return s;
}

int ta_hapi_rec_init(struct filenode *tdir, unsigned offset) {
	struct dir_head head;
	struct dir_entry entry;
	struct file_entry file;
	struct filenode *node;
	char name[MAX_FILENAME];
	int ret;

	fseek(fs->file, offset, SEEK_SET);
	ta_hapi_read((unsigned char*)&head, sizeof(struct dir_head), fs->file);

	for (int i = 0; i < head.entries; i++, head.offset += sizeof(struct dir_entry)) {
		fseek(fs->file, head.offset, SEEK_SET);
		ta_hapi_read((unsigned char*)&entry, sizeof(struct dir_entry), fs->file);

		fseek(fs->file, entry.name, SEEK_SET);
		ta_hapi_fgets0(name, MAX_FILENAME, fs->file);

		if (entry.flags == 1) {
			node = generic_add_file(tdir, name, FILETYPE_DIR);
			if (!node) {
				fprintf(stderr, "Error adding file desciption to library.\n");
				return errno;
			}

			ret = ta_hapi_rec_init(node, entry.offset);
			if (ret) {
				return ret;
			}
		} else {
			node = generic_add_file(tdir, name, FILETYPE_REGULAR);
			if (!node) {
				fprintf(stderr, "Error adding file desciption to library.\n");
				return errno;
			}

			fseek(fs->file, entry.offset, SEEK_SET);
			ta_hapi_read((unsigned char*)&file, sizeof(struct file_entry), fs->file);

			node->size = file.size;
			node->offset = file.offset;
			node->local_flags = file.flags;
		}
	}

	return 0;
}

void ta_hapi_unlz77(unsigned char *dest, unsigned char *src, int len) {
	unsigned char window[4096];
	int mask = 1;
	int tag = *src++;
	int n;
	int offset;
	int inptr, outptr = 1;

	while(len > 0) {
		if (tag & mask) {
			offset =  *((unsigned short *)src);
			n = (offset & 0xF) + 2;
			inptr = offset >>= 4;
			src += 2;
			len -= n;
			if (len < 0) {
				n += len;
			}
			for (int i = 0; i < n; i++) {
				*dest++ = window[inptr];
				window[outptr++] = window[inptr++];
				inptr %= 0x1000;
				outptr %= 0x1000;
			}
		} else {
			window[outptr++] = *src;
			outptr %= 0x1000;
			*dest++ = *src++;
			len--;
		}

		mask <<= 1;
		if (mask >= 0x100) {
			mask = 1;
			tag = *src++;
		}
	}
}

int ta_hapi_read_compressed(struct filenode *node) {
	struct chunk_header header;
	unsigned char chunk[2 * 65536];
	int count = ((node->size + 65535) >> 16);
	unsigned long length;

	fseek(fs->file, node->offset + (sizeof(unsigned) * count), SEEK_SET);

	for (int i = 0; i < count; i++) {
		ta_hapi_read((unsigned char *)&header, sizeof(struct chunk_header), fs->file);
		if (memcmp(header.magic, "SQSH", 4)) {
			return -EINVAL;
		} else if ((header.packed > (2 * 65536)) || (header.unpacked > 65536)) {
			return -ENOMEM;
		}

		ta_hapi_read(chunk, header.packed, fs->file);
		for (int j = 0; j < header.packed; j++) {
			chunk[j] = (chunk[j] - j) ^ j;
		}

		switch (header.compression) {
			case 1:
				ta_hapi_unlz77(&((unsigned char *)node->data)[i * 65536], chunk, header.unpacked);
				break;
			case 2:
				length = header.unpacked;
				uncompress(&((unsigned char *)node->data)[i * 65536], &length, chunk, header.packed);
				break;
			default:
				return -EINVAL;
		}
	}
	return 0;
}

int ta_hapi_fo_open(const char *path, struct fuse_file_info *fi) {
	struct filenode *node = NULL;
	int ret;

	node = generic_find_path(fs->root, path);
	if (!node) {
		return -ENOENT;
	}

	if ((fi->flags & 3) != O_RDONLY) {
		return -EACCES;
	}

	if (generic_is_dir(node)) {
		return -EISDIR;
	}

	pthread_mutex_lock(&fs->mutex);
	node->data = malloc(node->size);
	if (!node->data) {
		pthread_mutex_unlock(&fs->mutex);
		return -ENOMEM;
	}

	switch (node->local_flags) {
		case 0: //plain
			fseek(fs->file, node->offset, SEEK_SET);
			ta_hapi_read(node->data, node->size, fs->file);
			ret = 0;
			break;
		case 1: //LZ77
		case 2: //Zlib
			ret = ta_hapi_read_compressed(node);
			break;
		default:
			ret = -EINVAL;
			break;
	}
	pthread_mutex_unlock(&fs->mutex);
	return ret;
}

int init_game_ta_hapi(void) {
	unsigned char magic[4];
	struct private *priv;
	unsigned offset;
	int ret;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, sizeof(char), 4, fs->file);
	if (memcmp(magic, "HAPI", 4)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}

	priv = (struct private *)malloc(sizeof(struct private));
	if (!priv) {
		fprintf(stderr, "Not enough memory for private data.\n");
		return -ENOMEM;
	}
	fs->priv = priv;

	fseek(fs->file, 8, SEEK_CUR);
	fread(&priv->key, sizeof(unsigned), 1, fs->file);
	fread(&offset, sizeof(unsigned), 1, fs->file);
	if (priv->key) {
		priv->key = ~((priv->key * 4) | (priv->key >> 6));
		priv->is_encrypted = true;
	} else {
		priv->is_encrypted = false;
	}

	ret = ta_hapi_rec_init(fs->root, offset);
	if (ret) {
		return ret;
	}

	fs->operations.open = ta_hapi_fo_open;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;

	fs->fs_size = generic_subtree_size(fs->root);

	return 0;
}
