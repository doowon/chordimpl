#include <stdio.h>
#include "util.h"
#include "chord.h"

int main(int argc, char *argv[]) {
	// char* mdString = hashSHA1("hhhh");
	// printf("%s\n", mdString);
	// free(mdString);

	int port = 50000;

	int numNodes = 3;
	int id[3] = {0, 1, 3};

	//create chord nodes
	int i = 0;
	createNodes(3, id);
	print();
	return 0;
}