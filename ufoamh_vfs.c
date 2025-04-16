#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <zlib.h>

#include "generic.h"

struct private {
	unsigned cluster_size;
	unsigned clusters_defined;
	unsigned clusters_used;
	unsigned root_dir_count;
	unsigned data_start;
};

int ufoamf_vfs_read_to_buf(unsigned cluster, unsigned size, unsigned char *buf) {
	struct private *priv = (struct private *)fs->priv;
	unsigned cl = cluster;
	unsigned toread;

	while ((cl != 0xFFFFFFFF) && (size > 0)) {
		if (cl > priv->clusters_used) {
			return -EINVAL;
		}
		cl--;
		fseek(fs->file, priv->data_start + (priv->cluster_size * cl), SEEK_SET);
		toread = (size < priv->cluster_size) ? size : priv->cluster_size;
		fread(buf, sizeof(unsigned char), toread, fs->file);
		buf += toread;
		size -= toread;

		fseek(fs->file, 0x138 + (8 * cl), SEEK_SET);
		fread(&cl, sizeof(unsigned), 1, fs->file);
	}
	return 0;
}

int ufoamh_vfs_rec_init(struct filenode *tdir, unsigned char *buf, unsigned max) {
	char name[65];
	struct filenode *node;
	unsigned char *tmpbuf;
	unsigned size;
	int ret;

	for (int i = 0; (i < max) && (buf[0] != '\0'); i++, buf += 0x58) {
		memset(name, 0 ,sizeof(name));
		memcpy(name, buf, sizeof(name) - 1);
//		printf("%s - cl type %hhi, start: 0x%08x size: 0x%08x packed: 0x%08x\n", name, buf[68], *((unsigned *)&buf[76]), *((unsigned *)&buf[80]), *((unsigned *)&buf[84]));
		switch (buf[68]) {
			case 2:
				node = generic_add_file(tdir, name, FILETYPE_DIR);
				if (!node) {
					fprintf(stderr, "Error adding file desciption to library.\n");
					return errno;
				}

				size = *((unsigned *)&buf[80]);
				tmpbuf = (unsigned char *)malloc(size);
				if (!tmpbuf) {
					fprintf(stderr, "Not enough memory for temporary buffer.\n");
					return -ENOMEM;
				}

				ret = ufoamf_vfs_read_to_buf(*((unsigned *)&buf[76]), size, tmpbuf);
				if (ret) {
					free(tmpbuf);
					return -ENOMEM;
				}

				ret = ufoamh_vfs_rec_init(node, tmpbuf, size / 0x58);
				free(tmpbuf);
				if (ret) {
					return -ENOMEM;
				}
				break;
			case 1:
				node = generic_add_file(tdir, name, FILETYPE_REGULAR);
				if (!node) {
					fprintf(stderr, "Error adding file desciption to library.\n");
					return errno;
				}
				node->cluster = *((unsigned *)&buf[76]);
				node->size = *((unsigned *)&buf[80]);
				break;
			case 9:
				node = generic_add_file(tdir, name, FILETYPE_PACKED);
				if (!node) {
					fprintf(stderr, "Error adding file desciption to library.\n");
					return errno;
				}
				node->cluster = *((unsigned *)&buf[76]);
				node->packed = *((unsigned *)&buf[80]);
				node->size = *((unsigned *)&buf[84]);
				break;
			default:
				fprintf(stderr, "Unexpected file format %hhi.\n", buf[64]);
				return -EINVAL;
		}
	}
	return 0;
}

int ufoamh_vfs_fo_open(const char *path, struct fuse_file_info *fi) {
	struct filenode *node = NULL;
	unsigned long length;
	int left;
	unsigned chunk_size;
	unsigned char *packed, *orig;
	unsigned char *unpacked;

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
	node->data = malloc(node->size + 50000);
	orig = packed = malloc(node->packed);
	if (!node->data || !packed) {
		if (node->data) {
			free(node->data);
			node->data = NULL;
		}
		if (packed) {
			free(packed);
		}
		pthread_mutex_unlock(&fs->mutex);
		return -ENOMEM;
	}

	length = node->size;
	fseek(fs->file, node->offset, SEEK_SET);
	if (generic_is_type(node, FILETYPE_PACKED)) {
		ufoamf_vfs_read_to_buf(node->cluster, node->packed, packed);
		left = node->size;
		unpacked = node->data;
		while (left > 0) {
			length = (left < 50000) ? left : 50000;
			chunk_size = *((unsigned*)packed);
			packed += 4;
			uncompress(unpacked, &length, packed, chunk_size);

			packed += chunk_size;
			left -= length;
			unpacked += length;
		}
	} else {
		ufoamf_vfs_read_to_buf(node->cluster, node->size, node->data);
	}

	node->open_no++;
	pthread_mutex_unlock(&fs->mutex);
	free(orig);
	return 0;
}

int init_game_ufoamh_vfs(void) {
	unsigned char magic[4];
	unsigned char *buf;
	struct private *priv;
	int ret;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, sizeof(char), 4, fs->file);
	if (memcmp(magic, "\0\0\x80\x3F", 2)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}

	priv = (struct private *)malloc(sizeof(struct private));
	if (!priv) {
		fprintf(stderr, "Not enough memory for private data.\n");
		return -ENOMEM;
	}
	fs->priv = priv;

	fread(&priv->cluster_size, sizeof(unsigned), 1, fs->file);
	fread(&priv->clusters_defined, sizeof(unsigned), 1, fs->file);
	fread(&priv->root_dir_count, sizeof(unsigned), 1, fs->file);
	fseek(fs->file, 0x130, SEEK_SET);
	fread(&priv->clusters_used, sizeof(unsigned), 1, fs->file);
	priv->data_start = 0x134 + (priv->clusters_defined * 8) + (priv->root_dir_count * 0x58);

	buf = (unsigned char *)malloc(priv->root_dir_count * 0x58);
	if (!buf) {
		fprintf(stderr, "Not enough memory for temporary buffer.\n");
		return -ENOMEM;
	}
	fseek(fs->file, 0x134 + (priv->clusters_defined * 8), SEEK_SET);
	fread(buf, sizeof(unsigned char), priv->root_dir_count * 0x58, fs->file);

	ret = ufoamh_vfs_rec_init(fs->root, buf, priv->root_dir_count);
	free(buf);
	if (ret) {
		return ret;
	}

	fs->operations.open = ufoamh_vfs_fo_open;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}
