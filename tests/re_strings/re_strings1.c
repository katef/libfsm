#include "testutil.h"

const char *strings[] = {
	"aa",
	"ab",
	"ac",
	"ba",
	"bb",
	"bc",
	"ca",
	"cb",
	"cc",
	NULL,
};

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	return run_test(strings);
}
