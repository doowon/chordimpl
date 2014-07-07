/**@file util.h
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#ifndef UTIL_H
#define UTIL_H

#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char* hashSHA1(char* string);

#endif