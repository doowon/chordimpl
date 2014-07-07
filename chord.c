/**@file chord.c
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#include "chord.h"


int nowNodeListSize = 0;
int KEYS[MAX_NUM_NODE] = {1, 2, 6};
int nodeSize = 3;
int nodeArray[3] = {0, 1, 3};
node* nodeList[MAX_NUM_NODE];

int createNodes(int numNode, int id[]){

	if (numNode <=0 && numNode > MAX_NUM_NODE) {
		return -1;
	}

	int i = 0;
	for (i = 0; i < numNode; ++i)
		createNode(id[i]);
	assignKey();
	return 0;
}

int createNode(int id) {
	//TODO: free node
	node* nd = malloc(sizeof(node));
	nd->id = id;
	nodeList[nowNodeListSize++] = nd;
	
	//TODO: fingerTable
	createFingerTable(id);
	
	return 0;
}

int createFingerTable(int id) {
	int num = findId(id);
	if (num < 0)
		return -1;
	node* nd = nodeList[num];

	int i, j = 0;
	for (i = 0; i < FT_SIZE; ++i) {
		int p = (int)pow(2, i);
		nd->ft[i].node = (id + p) % (int)pow(2,FT_SIZE);
		for (j = 0; j < nodeSize; ++j) {
			if (nd->ft[i].node <= nodeArray[j]) {
				nd->ft[i].succesor = nodeArray[j];
				break;
			}
		}
	}
}

int findId(int id) {
	int i = 0;
	for (i = 0; i < MAX_NUM_NODE; ++i) {
		if(nodeList[i]->id == id)
			return i;
	}
	return -1;
}

int assignKey() {
	int i,j = 0;
	int num = 0;
	for (i = 0; i < MAX_NUM_NODE; ++i) {
		for (j = i; j < MAX_NUM_NODE; ++j) {
			if (KEYS[i] <= nodeList[j]->id) {
				nodeList[j]->key[0] = KEYS[i];
				num++;
				break;
			}
		}
	}
	if (num == MAX_NUM_NODE) return 0;

	//assign left keys to the first node
	j = 0;
	for(i = num; i < MAX_NUM_NODE; ++i) {
		nodeList[j]->key[j++] = KEYS[i];
	}	

	return 0;
}

int findSuccesor(node* nd, int id) {
	int n = findPredecessor(nd, id);
	return n.succesor;
}

int findPredecessor(node* nd, int id) {
	int i = 0;
	
}

int closestPrecedingFinger(node* nd, int id) {
	int i = 0;
	for (i = FT_SIZE-1; i >= 0; --i) {
		if(nd->ft[i].node <= id) {
			return nd->ft[i].node;
		}
	}
	return nd->ft[FT_SIZE-1].node;
}


/**
 * For debug
 */
void print() {
	int i, j = 0;
	for (i = 0; i < MAX_NUM_NODE; ++i) {
		// printf("NodeId %d\n", nodeList[i]->id);
		printf("NodeId %d, Key %d\n", nodeList[i]->id, nodeList[i]->key[0]);
		for (j = 0; j < FT_SIZE; ++j) {
			printf("FT %d -> %d\n", 
				nodeList[i]->ft[j].node, nodeList[i]->ft[j].succesor);
		}
	}
}