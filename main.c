#include <stdio.h>
#include <getopt.h>
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

	// if (argc != 2) {
	// 	printf("Usage: chord nodeId\n");
	// 	return -1;
	// }
	if (argc > 3 && argc < 2) {
		printf("Usage: chord -n nodeid -f -i\n");
		return -1;
	}
	
	int c = 0;
	bool failure = false;
	int fTime = 0;
	int nodeId = 0;
	bool interactive = false;
	while ((c = getopt(argc, argv, "f:n:i")) != -1) {
		switch (c) {
		case 'f': 
			failure = true; 
			fTime = atoi(optarg);
			break;
		case 'n':
			nodeId = atoi(optarg);
			break;
		case 'i':
			interactive = true;
			break;
		default:
			printf("Usage: chord -n nodeid -f -i\n");
			return -1;
		}
	}
	
	if (nodeId > 0){
		if (failure && !interactive) {
			initNode(nodeId, fTime);
		} else if (!failure && !interactive) {
			initNode(nodeId, 0);
		} else if (!failure && interactive) {
			printf("interative\n");
		}
	}

	return 0;
}