/*
 * Copyright 2026 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int fsm_main(const char *s, const unsigned **ids, size_t *count);

int main(void) {
	char buf[1024];
	const char *p;

	while (buf[sizeof buf - 1] = 'x', p = fgets(buf, sizeof buf, stdin), p != NULL) {
		const unsigned *ids;
		size_t count, i;
		int r;

		if (buf[sizeof buf - 1] == '\0' && buf[sizeof buf - 2] != '\n') {
			fprintf(stderr, "overflow\n");
			exit(EXIT_FAILURE);
		}

		buf[strcspn(buf, "\n")] = '\0';

		r = fsm_main(buf, &ids, &count);

		printf("%s: %smatch", buf, r == 0 ? "no" : "");

		if (r == 1) {
			printf(", ids:");
			for (i = 0; i < count; i++) {
				printf(" %u", ids[i]);
			}
		}

		printf("\n");
	}

	return 0;
}

