/** @file chord.c
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#include "chord.h"
#include "node.h"

/**
 * Initialize Chord
 * @param  nodeId     Node ID
 * @return            [description]
 */
int initChord(uint32_t nodeId) {
	nd = malloc(sizeof(Node));
	nd->ndInfo.id = nodeId;
	strcpy(nd->ndInfo.ipAddr, IP_ADDR);
	nd->ndInfo.port = DEFAULT_PORT + nodeId;
	nd->keySize = 0;		//initialize the number of keys
	nd->predInfo.id = 0;  	//init predecessor id 
	nd->predInfo.port = 0;  //init predecessor port
	
	// these three lines are for test,
	// TODO: keys should be given by parameters 
#if 0
	if (nodeId == 5) { 
		nd->key[0] = 2;
		nd->key[1] = nodeId;
		nd->keySize = 2;
	} else {
		nd->key[0] = nodeId;
		nd->keySize = 1;
	}
	// END - TEST LINE 
#endif

	//init the finger table
	int i = 0;
	for (i = 0; i < FT_SIZE; ++i) {
		int p = (int)pow(2, i);
		if ((nd->ndInfo.id + p) >= (int)pow(2,FT_SIZE))
			break;
		nd->ft[i].start = (nd->ndInfo.id + p) % (int)pow(2,FT_SIZE);
		nd->ft[i].sInfo.id = 0;
		nd->ft[i].sInfo.port = 0;
		if (i == 0) {
			nd->ft[i].sInfo.id = nd->ndInfo.id;
			nd->ft[i].sInfo.port = DEFAULT_PORT + nodeId;
		}
		strcpy(nd->ft[i].sInfo.ipAddr, nd->ndInfo.ipAddr);
		nd->ftSize++;
	}

	//intialize successor list
	for (i = 0 ; i < (int)pow(2, FT_SIZE); ++i) {
		nd->sList[i].info.id = 0;
		nd->sList[i].info.port = 0; //port 0 means a initial value
	}
	
	if (nodeId >= 2)
		join();

	return 0;
}


/**
 * [findSuccessor description]
 * @param  targetId    [description]
 * @param  successorId [description]
 * @param  ipAddr      [description]
 * @param  port        [description]
 * @return             
 */
int findSuccessor(uint32_t targetId, uint32_t* successorId, 
						char* ipAddr, uint16_t* port) {
	
	//look for id in LOCAL
	int n = closestPrecedingFinger(targetId, successorId, ipAddr, port);
	if (n == 2) { // no need to request more
		return 2;
	} else if (n < 0) {
		return -1;
	}

	// Look for targetId in remote
	while (1) {
		sendReqPkt(targetId, *successorId, ipAddr, *port);
		uint32_t recvTargetId = 0;
		n = recvResPkt(recvTargetId, successorId, ipAddr, port);
		if (n == 2) {
			return 2;
		} else if (n < 0) {
			return -1;
		}
	}

	return 0;
}

/**
 * Find the successor without looking for the local finger table
 * @param  targetId    [description]
 * @param  successorId [description]
 * @param  ipAddr      [description]
 * @param  port        [description]
 * @return             [description]
 */
int findInitSuccessor(uint32_t targetId, uint32_t* successorId,
						char* ipAddr, uint16_t* port) {
	while (1) {
		int n = sendReqPkt(targetId, *successorId, ipAddr, *port);
		if (n == -1) {
			return -1;
		}
		
		uint32_t recvTargetId = 0;
		n = recvResPkt(recvTargetId, successorId, ipAddr, port);
		if (n == 2) {
			return 2;
		}
	}

	return 0;
}

/**
 * [closestPrecedingFinger description]
 * @param  targetId    [description]
 * @param  successorId [description]
 * @param  ipAddr      [description]
 * @param  port        [description]
 * @return             1 if not found, 2 if found
 */
int closestPrecedingFinger(uint32_t targetId, uint32_t* successorId, 
								char* ipAddr, uint16_t* port) {

	if (targetId == nd->ndInfo.id)
		return 2;

	int i = 0;
	int ftSize = nd->ftSize;
	for (i = ftSize-1; i >= 0; --i) {
		if (nd->ft[i].sInfo.port != 0 && nd->ft[i].start <= targetId) {
			struct NodeInfo sNodeInfo = nd->ft[i].sInfo;
			*successorId = sNodeInfo.id;
			strcpy(ipAddr, sNodeInfo.ipAddr);
			*port = sNodeInfo.port;
			// No successors between targetId 
			// and the next sucessor
			if (targetId <= sNodeInfo.id || sNodeInfo.id <= nd->ndInfo.id)
				return 2;
			
			return 1;
		}
	}
	
	for (i = ftSize-1; i >= 0; --i) {
		if (nd->ft[i].sInfo.port != 0) {
			struct NodeInfo ndInfo = nd->ft[i].sInfo;
			*successorId = ndInfo.id;
			strcpy(ipAddr, ndInfo.ipAddr);
			*port = ndInfo.port;
			return 1;
		}
	}

	return -1;
}

/**
 * Join the network
 * @return [description]
 */
void join() {
	char ipAddr[15];
	strcpy(ipAddr, IP_ADDR);
	uint32_t sId = DEFAULT_NODE_ID;
	uint32_t targetId = nd->ndInfo.id;
	uint16_t port = DEFAULT_PORT + DEFAULT_NODE_ID;

	int n = findInitSuccessor(targetId, &sId, ipAddr, &port);
	if (n == 2) {
		nd->ft[0].sInfo.id = sId;
		strcpy(nd->ft[0].sInfo.ipAddr, ipAddr);
		nd->ft[0].sInfo.port = port;

		int i = 0;
		for (i = 0; i < nd->ftSize; ++i) {
			if (nd->ft[i].start <= sId) {
				nd->ft[i].sInfo.id = sId;
				strcpy(nd->ft[i].sInfo.ipAddr, ipAddr);
				nd->ft[i].sInfo.port = port;
			}
		}
	}
}

/**
 * Stablize its node
 */
void stabilize() {

	uint32_t sId = nd->ft[0].sInfo.id;
	uint16_t sPort = nd->ft[0].sInfo.port;
	char sIpAddr[15];
	strcpy(sIpAddr, nd->ft[0].sInfo.ipAddr);

	if (sId == 0 || sPort == 0) {
		return;
	}

	// keys transfer
	// ask the successor's keys
	uint32_t id = nd->ndInfo.id;
	uint32_t keys[NUM_KEYS];
	int num = 0;
	int n = 0;

	// To check to see if the successor fails
	n = checkConnection(sIpAddr, sPort);
	if (n < 0) {
		int i = 0;
		for (i = 1; i < (int)pow(2, FT_SIZE); ++i) {
			sId = nd->sList[i].sInfo.id;
			sPort = nd->sList[i].sInfo.port;
			strcpy(sIpAddr, nd->sList[i].sInfo.ipAddr);
			if (checkConnection(sIpAddr, sPort) >= 0)
				nd->ft[0].sInfo.id = sId;
				nd->ft[0].sInfo.port = sPort;
				strcpy(nd->ft[0].sInfo.ipAddr, sIpAddr);
				printFT();
				break;
		}
	}


	//ask succesor for its predecessor
	uint32_t predId = 0;
	uint16_t predPort = 0;
	char predIpAddr[15];
	
	n = askSuccForPred(sId, sIpAddr, sPort, &predId, predIpAddr, &predPort);
	if (n < 0) return;

	if (nd->ndInfo.id != predId) {
		//this node is not just joining & connect
		if (nd->predInfo.port != 0 && checkConnection(predIpAddr,predPort)>=0){ 
			nd->ft[0].sInfo.id = predId;
			strcpy(nd->ft[0].sInfo.ipAddr, predIpAddr);
			nd->ft[0].sInfo.port = predPort;
		}
		notify(nd->ndInfo);
		if (sId != nd->ndInfo.id) {
			n = askSuccForKeys(id, sId, sIpAddr, sPort, keys, &num);
			int i = 0;
			int size = nd->keySize;
			for (i = 0; i < num; ++i) {
				nd->key[size++] = keys[i];
			}
			nd->keySize += num;
		}
		// sort key array
		qsort(nd->key, nd->keySize, sizeof(int), cmpfunc);
	}
	
	printDebug();

	if (!(nd->ft[0].sInfo.id == 0 || nd->ft[0].sInfo.port == 0 ||
		nd->ft[0].sInfo.id == nd->ndInfo.id ||
		nd->ft[0].sInfo.port == nd->ndInfo.port ||
		nd->predInfo.id == 0)) {
		
		buildSuccessorList();
		printSuccList();
	}	
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

/**
 * Build the successor list
 */
void buildSuccessorList() {
	int i = 0;
	for (i = 0 ; i < (int)pow(2, FT_SIZE); ++i) {
		nd->sList[i].info.id = 0;
		nd->sList[i].info.port = 0;
		memset(nd->sList[i].info.ipAddr, 0, IPADDR_SIZE);

		nd->sList[i].sInfo.id = 0;
		nd->sList[i].sInfo.port = 0;
		memset(nd->sList[i].sInfo.ipAddr, 0, IPADDR_SIZE);

	}

	//TODO: check to see if the successor is alive.
	cpyNodeInfo(&(nd->ndInfo), &(nd->sList[0].info));
	cpyNodeInfo(&(nd->ft[0].sInfo), &(nd->sList[0].sInfo));

	char sIpAddr[15]; // the successor
	uint32_t sId = nd->sList[0].sInfo.id; // the successor
	strcpy(sIpAddr, nd->sList[0].sInfo.ipAddr);
	uint16_t sPort = nd->sList[0].sInfo.port; // the successor

	uint32_t ssId = 0; // the successor's successor
	char ssIpAddr[15]; // the successor's successor
	uint16_t ssPort = 0; // the successor's successor

	i = 1;
	while (nd->ndInfo.id != sId) {
		int n = askSuccForSucc(sId, sIpAddr, sPort, &ssId, ssIpAddr, &ssPort);
		if (n < 0) return;
		
		//successor
		nd->sList[i].info.id = sId;
		nd->sList[i].info.port = sPort;
		strcpy(nd->sList[i].info.ipAddr, sIpAddr);

		//sucessor's successor
		nd->sList[i].sInfo.id = ssId;
		nd->sList[i].sInfo.port = ssPort;
		strcpy(nd->sList[i++].sInfo.ipAddr, ssIpAddr);

		sId = ssId; 
		strcpy(sIpAddr, ssIpAddr);
		sPort = ssPort;
	}
}

void cpyNodeInfo(struct NodeInfo* src, struct NodeInfo* dst) {
	dst->id = src->id;
	dst->port = src->port;
	strcpy(dst->ipAddr, src->ipAddr);
}

/**
 * Ask the successor for keys
 * @param  id      [description]
 * @param  sId     [description]
 * @param  sIpAddr [description]
 * @param  sPort   [description]
 * @param  keys    the keys returned
 * @param  num     the number of keys returned
 * @return         [description]
 */
int askSuccForKeys(uint32_t id, uint32_t sId, char* sIpAddr, 
					uint16_t sPort, uint32_t keys[], int* num) {
	int n = sendAskSuccForKeyPkt(id, sId, sIpAddr, sPort);
	if (n < 0) return n;
	n = recvKeyResPkt(keys, num);
	if (n < 0) return n;

}

int askSuccForSucc(uint32_t sId, char* sIpAddr, uint16_t sPort,
					uint32_t* ssId, char* ssIpAddr, uint16_t* ssPort) {
	int n = sendAskSuccForSuccPkt(sId, sIpAddr, sPort);
	if (n < 0) return n;
	n = recvResPkt(sId, ssId, ssIpAddr, ssPort);
	if (n < 0) return n;
}

/**
 * Ask its sucessor for the successor's predecessor.
 * the predecessor information is returned as parameters
 * @param sId        [description]
 * @param sIpAddr    [description]
 * @param sPort      [description]
 * @param predId     [description]
 * @param predIpAddr [description]
 * @param predPort   [description]
 */
int askSuccForPred(uint32_t sId, char* sIpAddr, uint16_t sPort,
					uint32_t* predId, char* predIpAddr, uint16_t* predPort){

	int n = sendAskSuccForPredPkt(sId, sIpAddr, sPort);
	if (n < 0) return n;
	n = recvResPkt(sId, predId, predIpAddr, predPort);
	if (n < 0) return n;
}

/**
 * Notify its successor of this node as its predecessor
 * @param  pNodeInfo to be a predecessor of its successor 
 * @return           [description]
 */
void notify(struct NodeInfo pNodeInfo) {

	uint32_t sId = nd->ft[0].sInfo.id;
	uint16_t sPort = nd->ft[0].sInfo.port;
	char sIpAddr[15];
	strcpy(sIpAddr, nd->ft[0].sInfo.ipAddr);

	if ((nd->predInfo.port == 0) || nd->ndInfo.id == pNodeInfo.id) { 
		sendNotifyPkt(sId, sIpAddr, sPort, 
						nd->ndInfo.id, nd->ndInfo.ipAddr, nd->ndInfo.port);
	}
}

/**
 * Fix the finger table
 */
void fixFingers() {

	int i = 0;
	uint32_t sId = 0;
	uint16_t sPort = 0;
	char sIpAddr[15];
	uint32_t targetId = 0;
	
	for (i = 1; i < nd->ftSize; ++i) {
		targetId = nd->ft[i].start;
		if (targetId <= nd->ft[i-1].sInfo.id) {
			nd->ft[i].sInfo.id = nd->ft[i-1].sInfo.id;
			strcpy(nd->ft[i].sInfo.ipAddr, nd->ft[i-1].sInfo.ipAddr);
			nd->ft[i].sInfo.port = nd->ft[i-1].sInfo.port;
		} else {
			// ask its sucessor for the targer ID
			sId = nd->ft[0].sInfo.id;
			strcpy(sIpAddr, nd->ft[0].sInfo.ipAddr);
			sPort = nd->ft[0].sInfo.port;
			int n = findInitSuccessor(targetId, &sId, sIpAddr, &sPort);
			// if (sId != nd->ndInfo.id && n == 2) {
			if (n == 2) {
				nd->ft[i].sInfo.id = sId;
				strcpy(nd->ft[i].sInfo.ipAddr, sIpAddr);
				nd->ft[i].sInfo.port = sPort;
			}
		}
	}

	printFT();
}

/**
 * Get its predecessor
 * @param id     predecessor id
 * @param ipAddr predecessor ip address
 * @param port   predecessor port
 */
void getPredecesor(uint32_t* id, char* ipAddr, uint16_t* port) {
	*id = nd->predInfo.id;
	strcpy(ipAddr, nd->predInfo.ipAddr);
	*port = nd->predInfo.port;
}

void getSuccessor(uint32_t* id, char* ipAddr, uint16_t* port) {
	*id = nd->ft[0].sInfo.id;
	strcpy(ipAddr, nd->ft[0].sInfo.ipAddr);
	*port = nd->ft[0].sInfo.port;
}

/**
 * [getKeys description]
 * @param id   [description]
 * @param keys [description]
 * @param num  The number of the returned keys 
 */
void getKeys(uint32_t id, int keys[], int* num) {
	int i = 0; int j = 0;
	int size = nd->keySize;
	for (i = 0; i < size; ++i) {
		if (id >= nd->key[i] && nd->key[i] != nd->ndInfo.id) {
			keys[j++] = nd->key[i];
		}
	}
	if (j == 0) return;

	// remove keys from list
	// TODO: Make it sure that the keys transferred
	// because keys are removed here, but not transfer to predecessor
	int k = 0;
	for (i = j; i < size; ++i) {
		nd->key[k++] = nd->key[i];
	} 
	nd->keySize = size - j;
	*num = j;
}

/**
 * Modify its predecessor
 * @param id     predecessor id
 * @param ipAddr predecessor ip address
 * @param port   predecessor port
 */
void modifyPred(uint32_t id, char* ipAddr, uint16_t port) {
	nd->predInfo.id = id;
	nd->predInfo.port = port;
	strcpy(nd->predInfo.ipAddr, ipAddr);
}

void printDebug() {
	printf("Successor ID: %lu, Successor Port: %lu\n", 
		(unsigned long) nd->ft[0].sInfo.id, 
		(unsigned long) nd->ft[0].sInfo.port);
	printf("Predecessor ID: %lu, Predecessor Port: %lu\n", 
		(unsigned long) nd->predInfo.id, 
		(unsigned long) nd->predInfo.port);
	int i = 0;
	for (i = 0; i < nd->keySize; ++i) {
		printf("Keys: %lu\n", (unsigned long)nd->key[i]);
	}
	
}

void printFT() {
	int i = 0;
	for (i = 0; i < nd->ftSize; ++i) {
		printf("Start: %d \t Succ: %lu \t IP: %s \t Port: %lu\n", 
			nd->ft[i].start, 
			(unsigned long)nd->ft[i].sInfo.id,
			nd->ft[i].sInfo.ipAddr,
			(unsigned long)nd->ft[i].sInfo.port);
	}
}

void printSuccList() {
	int i = 0;
	for (i = 0 ; i < (int)pow(2, FT_SIZE); ++i) {
		printf("ID: %lu -> SID: %lu \n", 
			(unsigned long)nd->sList[i].info.id,
			(unsigned long)nd->sList[i].sInfo.id);
		 
	}
}