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

	if (argc > 3 && argc < 2) {
		printf("Usage: chord -p portNum -f fileName-t -i\n");
		return -1;
	}
	
	int c = 0;
	int fTime = 0;
	int port = 0;
	bool interactive = false;
	char* fileName = NULL;
	while ((c = getopt(argc, argv, "f:t:p:i")) != -1) {
		switch (c) {
		case 't': 
			fTime = atoi(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'i':
			interactive = true;
			break;
		case 'f':
			fileName = optarg;
			fprintf(stderr, "fileName: %s\n", fileName);
			break;
		default:
			printf("Usage: chord -p portNum -t -i\n");
			return -1;
		}
	}
	
	if (port > 0){
		initNode(fileName, port, fTime, interactive);
	}
	free(fileName);
	return 0;
}