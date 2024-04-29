#include "testutil.h"

const char *strings[] = {
	"first",
	"duplicate",
	"duplicate",
	"duplicate",
	"last",
	NULL,
};

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	return run_test(strings);
}
