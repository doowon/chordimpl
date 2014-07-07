/** @file node.h
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#ifndef NODE_H
#define NODE_H

#define FT_SIZE 3

struct _Node;
typedef struct _Node Node;

struct FingerTable{
	int start; ///(n + 2^(k-1))
	Node* successor;
};

struct _Node{
	int id;
	int key[10];
	struct FingerTable ft[FT_SIZE];
};



#endif