/** @file node.h
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#ifndef NODE_H
#define NODE_H

// struct fingerTable;
// struct node;
#define FT_SIZE 3

struct fingerTable{
	int start;
	int intervalStart;
	int intervalEnd;
	int node;
	int succesor;
};

typedef struct {
	int id;
	int key[10];
	struct fingerTable ft[FT_SIZE];
} node;


int join();
int get(int key);
int put(int key, int value);
int findSucessor(int key);
int findPredecessor(int);
int closestPrecedingFinger(int);


#endif