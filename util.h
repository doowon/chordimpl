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

void hashSHA1(char* string, unsigned char* digest);
void hashToString(unsigned char* digest, char* mdString);
void addValueToHash(unsigned char* digest, unsigned char* value, unsigned char* ret);
void power2(int i, unsigned char* ret);
int cmpHashValue(const unsigned char* v1, const unsigned char* v2);
int cmpfunc(const void* a, const void* b);

#endif