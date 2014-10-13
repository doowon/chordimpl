/**@file util.c
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#include "util.h"
#include "node.h"

void mpzToByteArray(mpz_t value, unsigned char* ret) {
	// char* str = mpz_get_str(NULL, 16, value);
	// assert(strlen(str) == SHA_DIGEST_LENGTH*2);

	// int i = 0;
	// for (i = SHA_DIGEST_LENGTH*2 - 1; i >= 0 ; i--) {
	// }
	// free(str);
	if (mpz_cmp_ui(value, 0) == 0) {
		memset(ret, 0, SHA_DIGEST_LENGTH);
		return;
	}
	mpz_export(ret, NULL, 1, sizeof(char), 1, 0, value);
}

void ByteArrayToMpz(mpz_t value, unsigned char* ret) {
	// mpz_import(value, SHA_DIGEST_LENGTH, 1, sizeof(ret[0]), 1, 0, ret);
	char str[SHA_DIGEST_LENGTH*2+1];
	hashToString(ret, str);
	mpz_set_str(value, str, 16);
}

bool between(const mpz_t id, const mpz_t start, const mpz_t end) {

	unsigned char maxValue[SHA_DIGEST_LENGTH] = {[0 ... SHA_DIGEST_LENGTH-1] = 0xFF};
	mpz_t max; mpz_init(max);
	mpz_import(max, SHA_DIGEST_LENGTH, 1, sizeof(maxValue[0]), 1, 0, maxValue);
	mpz_t min; mpz_init(min);

	if (mpz_cmp(start, end) < 0 && mpz_cmp(start, id) <= 0 
		&& mpz_cmp(id, end) <= 0) {
		return true;
	} else if ((mpz_cmp(start, end) > 0) && 
				((mpz_cmp(start, id) <= 0 && mpz_cmp(id, max) <= 0)
				|| (mpz_cmp(min, id) <= 0 && mpz_cmp(id, end) <= 0))) {
		return true;
	} else if (mpz_cmp(start, end) == 0 && mpz_cmp(id, end) == 0) {
		return true;
	}

	mpz_clear(max); mpz_clear(min);
	return false;
}

void hashSHA1(char* string, char* mdString) {
	assert(string); assert(mdString);
	unsigned char digest[SHA_DIGEST_LENGTH] = {0x00,};
	SHA1((unsigned char*) string, strlen(string), digest);
	hashToString(digest, mdString);
}

void hashToString(unsigned char* digest, char* mdString) {
	assert(digest); assert(mdString);
	int i = 0;
	for (i = 0; i < SHA_DIGEST_LENGTH; ++i)
		sprintf((char*)&mdString[i*2], "%02x", digest[i]); 
	mdString[i*2+1] = '\0';
}


/**
 * Compare function for the quicksort
 * @param  a [description]
 * @param  b [description]
 * @return   [description]
 */
int cmpfunc(const void* a, const void* b) {
	return (*(int*)a - *(int*)b);
	// KeyData* k1 = (struct KeyData*) a; 
	// KeyData* k2 = (struct KeyData*) b;

	// return mpz_cmp(k1->key, k2->key); 
}

void freeStr(char* ptr) {
	if (ptr != NULL) {
		free(ptr);
	}
}