#include <stdio.h>
#include "util.h"
#include "chord.h"
#include "node.h"

#define DEBUG 1

int main(int argc, char *argv[]) {
	// char* mdString = hashSHA1("hhhh");
	// printf("%s\n", mdString);
	// free(mdString);

#if 0
	int port = 50000;

	int numNodes = 3;
	int nodeId[3] = {0, 1, 3};

	//create chord nodes
	int i = 0;
	createNodes(3, nodeId);
	print();
#endif
	if (argc != 2) {
		printf("error\n");
		return -1;
	}

	uint32_t nodeId = atoi(argv[1]);

	initNode(nodeId);
	// initNode(1);
	// initNode(3);

	return 0;
}