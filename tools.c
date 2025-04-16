#include <stdint.h>

void pathDosToUnix(char *path, uint32_t len) {
	for (uint32_t i = 0; i < len; i++) {
		if (path[i] == '\\') {
			path[i] = '/';
		}
	}
}

void strrev(char *str, uint32_t len) {
	char tmp;
	for (uint32_t i = 0; i < (len / 2); i++) {
		tmp = str[len - 1 - i];
		str[len - 1 - i] = str[i];
		str[i] = tmp;
	}
}