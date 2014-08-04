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

	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("mutex init failed\n");
		abort();
	}

	nd = malloc(sizeof(Node));
	nd->ndInfo.id = nodeId;
	strcpy(nd->ndInfo.ipAddr, IP_ADDR);
	nd->ndInfo.port = DEFAULT_PORT + nodeId;
	nd->keySize = 0;		//initialize the number of keys
	nd->predInfo.id = 0;  	//init predecessor id 
	nd->predInfo.port = 0;  //init predecessor port

	// these three lines are for test,
	// TODO: keys should be given by parameters
	// key assigned to node 1
	int i = 0;
	srand(time(NULL));
	if (nodeId == 1) {
		printf("Keys:");
		for (i = 0; i < 10; i++) {
			nd->key[i] = rand() % NUM_KEYS;
			nd->keySize++;
			printf(" %lu", (unsigned long) nd->key[i]);
		}
		printf("\n");
	}

	//init the finger table
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
	while (true) {
		char ipAddr[15];
		strcpy(ipAddr, IP_ADDR);
		uint32_t sId = DEFAULT_NODE_ID;
		uint32_t targetId = nd->ndInfo.id;
		uint16_t port = DEFAULT_PORT + DEFAULT_NODE_ID;

		uint32_t predId = 0;
		uint16_t predPort = 0;
		char predIpAddr[15];
		
		int n = findInitSuccessor(targetId, &sId, ipAddr, &port);		
		if (n == 2) {
			n = askSuccForPred(sId, ipAddr, port, 
								&predId, predIpAddr, &predPort);
#if debug
printf("[Join] Found SID %lu, PID %lu \n",(unsigned long)sId, (unsigned long)predId);
#endif 
			if (n < 0) {
				printf("Join Failed");
				return;
			} else if (predId > nd->ndInfo.id) {
				usleep(500 * 1000);
				continue;
			}

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
		break;
	}
}

/**
 * Stablize its node
 */
void stabilize() {
#if debug
printf("Stablizing start\n");
#endif
	uint32_t sId = nd->ft[0].sInfo.id;
	uint16_t sPort = nd->ft[0].sInfo.port;
	char sIpAddr[15];
	strcpy(sIpAddr, nd->ft[0].sInfo.ipAddr);
	int n = 0;
	
	if (sId == 0 || sPort == 0) {
		return;
	}

	// To check to see if the successor fails
#if debug
	printf("check connection\n");
#endif
	n = checkConnection(sIpAddr, sPort);
	if (n < 0) {
#if debug
		printf("Connection Failed\n");
#endif
		int i = 0;
		for (i = 1; i < (int)pow(2, FT_SIZE); ++i) {
			if (nd->sList[i].sInfo.id <= 0)
				return;
			sId = nd->sList[i].sInfo.id;
			sPort = nd->sList[i].sInfo.port;
			strcpy(sIpAddr, nd->sList[i].sInfo.ipAddr);
			if (checkConnection(sIpAddr, sPort) >= 0)
				nd->ft[0].sInfo.id = sId;
				nd->ft[0].sInfo.port = sPort;
				strcpy(nd->ft[0].sInfo.ipAddr, sIpAddr);
				// printFT();
				break;
		}
	}

	//ask succesor for its predecessor
	uint32_t predId = 0;
	uint16_t predPort = 0;
	char predIpAddr[15];
	
	n = askSuccForPred(sId, sIpAddr, sPort, &predId, predIpAddr, &predPort);
	if (n < 0) {
#if debug
printf("askSuccForPred error\n");
#endif
		return;
	}
#if debug
printf("[Stabilzing] PredID1: %lu SID: %lu\n", (unsigned long) predId, (unsigned long) sId);
#endif
	if (nd->ndInfo.id != predId) {
		//this node is not just joining & connect
		if (nd->predInfo.port != 0 && checkConnection(predIpAddr,predPort)>=0){ 
#if debug
printf("[Stabilzing] PredID2: %lu\n", (unsigned long) predId);
#endif
			nd->ft[0].sInfo.id = predId;
			strcpy(nd->ft[0].sInfo.ipAddr, predIpAddr);
			nd->ft[0].sInfo.port = predPort;
		}

		notify(nd->ndInfo);

		// keys transfer
		// ask the successor's keys
		uint32_t id = nd->ndInfo.id;
		uint32_t keys[NUM_KEYS];
		int num = 0;

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
#if debug
	printDebug();
#endif
	if (!(nd->ft[0].sInfo.id == 0 || nd->ft[0].sInfo.port == 0 ||
		nd->ft[0].sInfo.id == nd->ndInfo.id ||
		nd->ft[0].sInfo.port == nd->ndInfo.port ||
		nd->predInfo.id == 0)) {
		
		buildSuccessorList();
#if debug
		printSuccList();
#endif
		fixFingers();
	}
#if debug
printf("Stablizing end\n");
#endif
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
#if debug
printf("buildSuccessorList Starting..\n");
#endif
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

#if debug
printf("%i %s %lu %s %lu\n", __LINE__, __FILE__, (unsigned long) sId, sIpAddr, (unsigned long) sPort);
#endif
		ssId = 0; memset(ssIpAddr,0,15); ssPort = 0;
		int n = askSuccForSucc(sId, sIpAddr, sPort, &ssId, ssIpAddr, &ssPort);
		if (n < 0) { //connection fail or can't find
			sId = nd->sList[i-1].info.id;
			sPort = nd->sList[i-1].info.port;
			strcpy(sIpAddr, nd->sList[i-1].info.ipAddr);

#if debug
printf("%i %s %lu %s %lu\n", __LINE__, __FILE__, (unsigned long) sId, sIpAddr, (unsigned long) sPort);
#endif
			usleep(100 * 1000);
			continue;
		}
		
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

#if debug
printf("buildSuccessorList Ending..\n");
#endif
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

	return 0; //success

}

int askSuccForSucc(uint32_t sId, char* sIpAddr, uint16_t sPort,
					uint32_t* ssId, char* ssIpAddr, uint16_t* ssPort) {
	int n = sendAskSuccForSuccPkt(sId, sIpAddr, sPort);
	if (n < 0) return n;
	n = recvResPkt(sId, ssId, ssIpAddr, ssPort);
	if (n < 0) return n;

	return 0; //success
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

	return 0; //success
}

/**
 * Notify its successor of this node as its predecessor
 * @param  pNodeInfo to be a predecessor of its successor 
 * @return           [description]
 */
void notify(struct NodeInfo pNodeInfo) {
	pthread_mutex_lock(&lock);
	uint32_t sId = nd->ft[0].sInfo.id;
	uint16_t sPort = nd->ft[0].sInfo.port;
	char sIpAddr[15];
	strcpy(sIpAddr, nd->ft[0].sInfo.ipAddr);
	uint32_t pid = nd->predInfo.id;

	if ((pid == 0) || nd->ndInfo.id == pNodeInfo.id) {
#if debug
printf("[Notify] targetId: %lu myID: %lu\n", (unsigned long)sId, (unsigned long)nd->ndInfo.id);
#endif
		sendNotifyPkt(sId, sIpAddr, sPort, 
						nd->ndInfo.id, nd->ndInfo.ipAddr, nd->ndInfo.port);
	}
	pthread_mutex_unlock(&lock);
}

/**
 * Fix the finger table
 */
void fixFingers() {
#if debug
printf("FixFingers Starts\n");
#endif

/* OLD VERSION: retrieving via tcp sockets*/
#if 1
	int i = 0;
	uint32_t sId = 0;
	uint16_t sPort = 0;
	char sIpAddr[15];
	uint32_t targetId = 0;

	for (i = 1; i < nd->ftSize; ++i) {
		targetId = nd->ft[i].start;
		if ((targetId <= nd->ft[i-1].sInfo.id)
				|| nd->ft[i-1].sInfo.id == 1){
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
#endif

/* This version is to use successList */
#if 0
	int i, j = 0;
	uint32_t targetId = 0;
	
	for (i = 1; i < nd->ftSize; ++i) {
		targetId = nd->ft[i].start;
		for (j = 0; (int)pow(2, FT_SIZE); ++j) {
			if ((nd->sList[j].sInfo.id != 0) 
					&& (targetId <= nd->sList[j].sInfo.id)) {
				nd->ft[i].sInfo.id = nd->sList[j].sInfo.id;
				strcpy(nd->ft[i].sInfo.ipAddr, "127.0.0.1");
				nd->ft[i].sInfo.port = nd->sList[j].sInfo.port;
				break;
			}
		}
		//when its next successor is the first (default) node 
		nd->ft[i].sInfo.id = DEFAULT_NODE_ID;
		strcpy(nd->ft[i].sInfo.ipAddr, "127.0.0.1");
		nd->ft[i].sInfo.port = DEFAULT_PORT+DEFAULT_NODE_ID;
	}
#endif
#if debug
	printFT();
printf("FixFingers Ends\n");
#endif
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
 * Get keys to be moved to the request node
 * @param id   the request node ID
 * @param keys keys to be moved to the request node
 * @param num  the number of the returned keys 
 */
void getKeys(uint32_t id, uint32_t keys[], int* num) {
	int i = 0; int j = 0;
	int size = nd->keySize;
	for (i = 0; i < size; ++i) {
		if (id >= nd->key[i] && nd->key[i] != nd->ndInfo.id) {
			keys[j++] = nd->key[i];
		}
	}
	if (j == 0) {
		return;
	}

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
pthread_mutex_lock(&lock);
	nd->predInfo.id = id;
	nd->predInfo.port = port;
	strcpy(nd->predInfo.ipAddr, ipAddr);
#if debug
printf("ModifyPred: %lu\n", (unsigned long) id);
#endif
pthread_mutex_unlock(&lock);
}

void printDebug() {
	printf("Predecessor ID: %lu, Predecessor Port: %lu\n", 
		(unsigned long) nd->predInfo.id, 
		(unsigned long) nd->predInfo.port);
	printf("Successor ID: %lu, Successor Port: %lu\n", 
		(unsigned long) nd->ft[0].sInfo.id, 
		(unsigned long) nd->ft[0].sInfo.port);
	int i = 0;
	printf("Keys:");
	for (i = 0; i < nd->keySize; ++i) {
		printf(" %lu", (unsigned long)nd->key[i]);
	}
	printf("\n");
	
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
	// for (i = 0 ; i < (int)pow(2, FT_SIZE); ++i) {
	printf("Successors: ");
	for (i = 0; i < 100; ++i) {
		// printf("ID: %lu -> SID: %lu \n", 
		// 	(unsigned long)nd->sList[i].info.id,
		// 	(unsigned long)nd->sList[i].sInfo.id);

		if ((unsigned long)nd->sList[i].sInfo.id == 0)
			break;
		printf("%lu ", (unsigned long)nd->sList[i].sInfo.id);
	}
	printf("\n");
}