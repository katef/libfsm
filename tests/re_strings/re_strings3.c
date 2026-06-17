#include "testutil.h"

const char *strings[] = {
	"duplicate",
	"duplicate",
	"duplicate",
	NULL,
};

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	return run_test(strings);
}
