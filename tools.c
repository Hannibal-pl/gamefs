#include <stdint.h>

void pathDosToUnix(char *path, uint32_t len) {
	for (uint32_t i = 0; i < len; i++) {
		if (path[i] == '\\') {
			path[i] = '/';
		}
	}
}
