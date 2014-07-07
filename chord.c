/**@file chord.c
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#include "chord.h"

int nodeListSize = 0;
int KEYS[MAX_NUM_NODE] = {1, 2, 6};
int nodeSize = 3;
int nodeIdArray[3] = {0, 1, 3};
Node* nodeList[MAX_NUM_NODE];

int createNodes(int numNode, int nodeId[]){

	if (numNode <=0 && numNode > MAX_NUM_NODE) {
		return -1;
	}

	int i = 0;
	for (i = 0; i < numNode; ++i) {
		createNode(nodeId[i]);
	}
	for (i = 0; i < numNode; ++i) {
		Node* nd = nodeList[i];
		buildFingerTable(nd);
	}
	assignKey();

	//test
	printf("Looking for %d at Node %d\n", 1, 0);
	Node* nd = findSuccessor(nodeList[0], 1);
	printf("found at Node %d\n",nd->id);
	return 0;
}

int createNode(int nodeId) {
	//TODO: free node
	Node* nd = malloc(sizeof(Node));
	nd->id = nodeId;
	nodeList[nodeListSize++] = nd;
	
	return 0;
}

int buildFingerTable(Node* nd) {
	int i, j = 0;
	for (i = 0; i < FT_SIZE; ++i) {
		int p = (int)pow(2, i);
		nd->ft[i].start = (nd->id + p) % (int)pow(2,FT_SIZE);
		for (j = nodeSize-1; j >= 0; --j) {
			if (nd->ft[i].start > nodeIdArray[j]) {
				nd->ft[i].successor = findId(nodeIdArray[(j+1)%FT_SIZE]);
				break;
			} else if (nd->ft[i].start == nodeIdArray[j]) {
				nd->ft[i].successor = findId(nodeIdArray[j]);
			}
		}
	}
	return 0;
}

Node* findId(int id) {
	int i = 0;
	for (i = 0; i < nodeSize; ++i) {
		if(nodeList[i]->id == id)
			return nodeList[i];
	}
	return 0;
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

Node* findSuccessor(Node* nd, int targetId) {
#ifdef DEBUG
	printf("at %d, Node id: %d, TagerId: %d\n", __LINE__, nd->id, targetId);
#endif
	nd = findPredecessor(nd, targetId);
	return nd; //the first one is its successor
}

Node* findPredecessor(Node* nd, int targetId) {

#ifdef DEBUG
	printf("at %d, Node id: %d, TagerId: %d\n", __LINE__, nd->id, targetId);
#endif

	bool circled = false;

	while (1) { 
		if (nd->id == targetId || circled) {
			return nd;
		} 
		int i = 0;
		for (i = 0; i < FT_SIZE ; ++i) {
			if (nd->ft[i].start == targetId)
				return nd->ft[i].successor;
		}

#ifdef DEBUG
	printf("at %d, Node id: %d, TagerId: %d\n", __LINE__, nd->id, targetId);
#endif
		nd = closestPrecedingFinger(nd, targetId, &circled);
	}
	return nd;
}

Node* closestPrecedingFinger(Node* nd, int targetId, bool* circled) {
	int i = 0;
	for (i = FT_SIZE-1; i >= 0; --i) {
		if(nd->ft[i].start <= targetId) {
			if ((nd->ft[i].successor)->id < nd->ft[i].start) {
				*circled = true;
				return nd->ft[i].successor;
			}
			return nd->ft[i].successor;
		}
	}
	return nd->ft[FT_SIZE-1].successor;
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
			int start = nodeList[i]->ft[j].start;
			Node* nd = nodeList[i]->ft[j].successor;
			printf("FT %d -> %d\n", start, nd->id);
		}
	}
}