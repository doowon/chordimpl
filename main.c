#include <stdio.h>
#include "util.h"
#include "chord.h"
#include "node.h"

/**
 * 
 */
int main(int argc, char *argv[]) {
	// char* mdString = hashSHA1("hhhh");
	// printf("%s\n", mdString);
	// free(mdString);

	// if (argc != 3) {
	// 	printf("Usage: chord nodeId finishTime\n");
	// 	return -1;
	// }

	if (argc != 2) {
		printf("Usage: chord nodeId\n");
		return -1;
	}

	uint32_t nodeId = atoi(argv[1]);
	// int finishTime = atoi(argv[2]);

	initNode(nodeId);

	return 0;
}