#include "testutil.h"

const char *strings[] = {
	/* empty */
	NULL,
};

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	return run_test(strings);
}
