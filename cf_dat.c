#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <syslog.h>

#include "cf_dat.h"

#define LEAF_COUNT 314
#define TREE_SIZE ((LEAF_COUNT << 1) - 1)

unsigned char offcode[256];
unsigned char offlen[8] = {0x00, 0x00, 0x00, 0x01, 0x05, 0x12, 0x30, 0x78};

struct node {
	unsigned nr;
	unsigned left;
	unsigned right;
	unsigned parent;
	unsigned value;
	unsigned weight;
	bool leaf;
};

struct node tree[TREE_SIZE];
unsigned char offcode[256];

void cf_dat_init_offcode(void) {
	memset(offcode, 0xFF, 256);
	for (unsigned i = 0, v = 0; i < 64; i++, v++) {
		if ((i == 1) || (i == 4) || (i == 12) || (i == 24) || (i == 48)) {
			v <<= 1;
		}
		offcode[v] = i;
	}
}

void cf_dat_init_tree(void) {
	int i;

	for(i = 0; i < LEAF_COUNT; i++) {
		tree[i].value = i;
		tree[i].nr = i;
		tree[i].weight = 1;
		tree[i].leaf = true;
	}
	for(i = LEAF_COUNT; i < TREE_SIZE; i++) {
		tree[i].value = i;
		tree[i].nr = i;
		tree[i].weight = tree[(i - LEAF_COUNT) << 1].weight + tree[((i - LEAF_COUNT) << 1) + 1].weight;
		tree[i].leaf = false;
		tree[i].left = (i - LEAF_COUNT) << 1;
		tree[i].right = ((i - LEAF_COUNT) << 1) + 1;
		tree[(i - LEAF_COUNT) << 1].parent = i;
		tree[((i - LEAF_COUNT) << 1) + 1].parent = i;
	}
}

int cf_dat_block_max(unsigned start) {
	int maxi = -1;
	int maxnr = tree[start].nr;
	for (unsigned i = 0; i != (TREE_SIZE - 1); i++) {
		if ((tree[i].nr > maxnr) && (tree[i].weight == tree[start].weight) && (tree[start].parent != i)) {
			maxi = i;
			maxnr = tree[i].nr;
		}
	}
	return maxi;
}

unsigned cf_dat_get_symbol(unsigned char *src, unsigned *src_bit) {
	bool bit;
	unsigned cur = TREE_SIZE - 1;
	unsigned ret, tmp;
	int max;

	while(!tree[cur].leaf) {
		bit = src[*src_bit >> 3] & (0x80 >> (*src_bit & 0x7));
		(*src_bit)++;

		if (bit) {
			cur = tree[cur].right;
		} else {
			cur = tree[cur].left;
		}
	}
	ret = tree[cur].value;
//	syslog(LOG_DEBUG, ": tree[%i] = %i\n", cur, ret);
	while(tree[cur].nr != (TREE_SIZE - 1)) {
		max = cf_dat_block_max(cur);
//		syslog(LOG_DEBUG, "MAX  %i\n", max);
		if (max >= 0) {
			if (tree[max].parent == tree[cur].parent) {
//				syslog(LOG_DEBUG, "rebalance twins[%i] %i <=> %i \n", tree[max].weight, cur, max);
				tmp = tree[tree[max].parent].left;
				tree[tree[max].parent].left = tree[tree[max].parent].right;
				tree[tree[max].parent].right = tmp;
			} else {
//				syslog(LOG_DEBUG, "rebalance[%i] %i <=> %i \n", tree[max].weight, cur, max);
				if (tree[tree[max].parent].left == max) {
					tree[tree[max].parent].left = cur;
				} else {
					tree[tree[max].parent].right = cur;
				}
				if (tree[tree[cur].parent].left == cur) {
					tree[tree[cur].parent].left = max;
				} else {
					tree[tree[cur].parent].right = max;
				}
				tmp = tree[max].parent;
				tree[max].parent = tree[cur].parent;
				tree[cur].parent = tmp;
			}

			tmp = tree[max].nr;
			tree[max].nr = tree[cur].nr;
			tree[cur].nr = tmp;
		}
		tree[cur].weight++;
		cur = tree[cur].parent;
	}
	tree[cur].weight++;
	return ret;
}

unsigned cf_dat_get_offset(unsigned char *src, unsigned *src_bit) {
	bool bit;
	unsigned i;
	unsigned tmp, ret;

	for(i = 0, tmp = 0; i < 8; i++) {
		if (tmp < offlen[i]) {
			break;
		}
		if (i) {
			tmp <<= 1;
		}
		bit = src[*src_bit >> 3] & (0x80 >> (*src_bit & 0x7));
		(*src_bit)++;
		if (bit) {
			tmp |= 1;
		}
	}
//	syslog(LOG_DEBUG, "LEN %i TMP 0x%02x\n", i, tmp);
	ret = offcode[tmp] << 6;
	for(i = 0, tmp = 0; i < 6; i++) {
		if (i) {
			tmp <<= 1;
		}
		(*src_bit)++;
		if (bit) {
			tmp |= 1;
		}
	}
	ret |= tmp;
	return ret;
}


int cf_dat_uncompress(unsigned char *dest, unsigned unpacked, unsigned char *src) {
	unsigned char buf[4096];
	unsigned bpos = 4039;
	unsigned src_bit = 0;
	unsigned symbol, offset, len;

	memset(buf, ' ', bpos);
	cf_dat_init_tree();
	while(unpacked) {
		symbol = cf_dat_get_symbol(src, &src_bit);
		if (symbol < 256) {
//			syslog(LOG_DEBUG, "UNP 0x%1$02X \"%1$c\".\n", symbol & 0xFF);
			*dest++ = symbol & 0xFF;
			buf[bpos++] = symbol & 0xFF;
			bpos = bpos & 0xFFF;
			unpacked--;
		} else {
//			syslog(LOG_DEBUG, "UNP 0x%1$03X.\n", symbol);
			offset = cf_dat_get_offset(src, &src_bit);
			len = symbol - 253;
//			syslog(LOG_DEBUG, "OFFSET %1$i 0x%1$04X  LEN %2$i.\n", offset, len);
			for(; len > 0; len--) {
				if ((bpos - offset - 1) < 0) {
					*dest++ = buf[bpos - offset + 4095];
					buf[bpos] = buf[bpos - offset + 4095];
				} else {
					*dest++ = buf[bpos - offset - 1];
					buf[bpos] = buf[bpos - offset - 1];
				}
				bpos++;
				bpos = bpos & 0xFFF;
				if (!--unpacked) {
					break;
				}
			}
		}
//		syslog(LOG_DEBUG, "UNPACKED %i.\n", unpacked);
	}
	return 0;
}

int cf_dat_fo_open(const char *path, struct fuse_file_info *fi) {
	struct filenode *node = NULL;
	void *packed;

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
	packed = malloc(node->packed);
	if (!packed) {
		pthread_mutex_unlock(&fs->mutex);
		return -ENOMEM;
	}

	fseek(fs->file, node->offset, SEEK_SET);
	node->data = malloc(node->size);
	if (!node->data) {
		free(packed);
		pthread_mutex_unlock(&fs->mutex);
		return -ENOMEM;
	}

	fread(packed, 1, node->packed, fs->file);
	cf_dat_uncompress(node->data, node->size, packed);
	free(packed);

	node->open_no++;
	pthread_mutex_unlock(&fs->mutex);
	return 0;
}

int init_game_cf_dat(void) {
	unsigned hsize;
	char name[13];
	struct filenode *node;
	unsigned char size;
	unsigned here;

	fseek(fs->file, 0, SEEK_SET);
	fread(&hsize, sizeof(unsigned), 1, fs->file);
	while (ftell(fs->file) < hsize) {
		memset(name, 0, sizeof(name));
		fread(&size, sizeof(unsigned char), 1, fs->file);
		fread(name, 1, size, fs->file);
		node = generic_add_file(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		fread(&node->offset, sizeof(unsigned), 1, fs->file);
		fread(&node->packed, sizeof(unsigned), 1, fs->file);
		here = ftell(fs->file);
		fseek(fs->file, node->offset, SEEK_SET);
		fread(&node->size, sizeof(unsigned), 1, fs->file);
		fseek(fs->file, here, SEEK_SET);
		node->offset += 4;
		node->packed -= 4;
	}

	fs->operations.open = cf_dat_fo_open;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;

	fs->fs_size = generic_subtree_size(fs->root);

	cf_dat_init_offcode();
	return 0;
}