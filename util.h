/**@file util.h
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#ifndef UTIL_H
#define UTIL_H

#include "node.h"
#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <gmp.h>

typedef int bool;
#define true 1
#define false 0

void ByteArrayToMpz(mpz_t value, unsigned char* ret);
void mpzToByteArray(mpz_t value, unsigned char* ret);
bool between(const mpz_t id, const mpz_t start, const mpz_t end, const mpz_t max, const mpz_t min);
void hashSHA1(char* string, char* digest);
void hashToString(unsigned char* digest, char* mdString);
void addValueToHash(unsigned char* digest, unsigned char* value, unsigned char* ret);
void power2(int i, unsigned char* ret);
int cmpHashValue(const unsigned char* v1, const unsigned char* v2);
int cmpfunc(const void* a, const void* b);
void freeStr(char* ptr);

#endif