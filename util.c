/**@file util.c
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#include "util.h"

char* hashSHA1(char* string) {
	unsigned char digest[SHA_DIGEST_LENGTH]; //20bytes
	char* mdString = malloc(SHA_DIGEST_LENGTH*2+1);

	memset(digest, 0x0, SHA_DIGEST_LENGTH);
	memset(mdString, 0x0, SHA_DIGEST_LENGTH);

	SHA1((unsigned char*) string, strlen(string), digest);

	int i = 0;
	for (i = 0; i < SHA_DIGEST_LENGTH; ++i)
		sprintf((char*)&mdString[i*2], "%02x", digest[i]); 

	// printf("%s\n",digest);
	return mdString;
}
