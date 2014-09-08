/**@file util.c
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#include "util.h"

void hashSHA1(char* string, unsigned char* digest) {
	assert(string); assert(digest);
	memset(digest, 0, SHA_DIGEST_LENGTH);
	SHA1((unsigned char*) string, strlen(string), digest);
}

void hashToString(unsigned char* digest, char* mdString) {
	assert(digest); assert(mdString);
	int i = 0;
	for (i = 0; i < SHA_DIGEST_LENGTH; ++i)
		sprintf((char*)&mdString[i*2], "%02x", digest[i]); 
	mdString[i*2+1] = '\0';
}

void addValueToHash(unsigned char* digest, unsigned char* value, unsigned char* ret) {
	assert(digest); assert(value); assert(ret);

	int i = 0; bool carry = false;
	for (i = SHA_DIGEST_LENGTH-1; i >= 0; --i) {
		if (carry) {
			ret[i] = digest[i] + value[i] + 0x01;
			carry = false;
		} else {
			ret[i] = digest[i] + value[i];
		}
		if ((unsigned int)(digest[i] + value[i]) > 255) {
			carry = true;
		} else if (carry && (unsigned int)(digest[i] + value[i] + 0x01) > 255) {
			carry = true;	
		}
	}
}

void power2(int i, unsigned char* ret) {
	assert(ret); assert(i >= 0); assert(i < FT_SIZE);

	int p = i / 8; // 8 bits
	ret[SHA_DIGEST_LENGTH - 1 - p] = 0x01 << (i % 8);
}

/**
 * Compare Hash Value
 * @param  v1 [description]
 * @param  v2 [description]
 * @return    less than 0 if v2 is bigger, 0 if same, more than 0 if v1 is bigger
 */
int cmpHashValue(const unsigned char* v1, const unsigned char* v2) {
	assert(v1); assert(v2);

	int i = 0;
	for (i = 0; i < SHA_DIGEST_LENGTH; ++i) {
		if (v1[i] - v2[i]) {
			return v1[i] - v2[i];
		}
	}
	return 0;
}

/**
 * Compare function for the quicksort
 * @param  a [description]
 * @param  b [description]
 * @return   [description]
 */
int cmpfunc(const void* a, const void* b) {
	return (*(int*)a - *(int*)b);
}

