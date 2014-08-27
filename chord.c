/** @file chord.c
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#include "chord.h"
#include "node.h"

/**
 * Initialize Chord
 * @param  nodeId     Node ID to be assigned 
 * @return            [description]
 */
int initChord(uint32_t nodeId) {

	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("mutex init failed\n");
		abort();
	}

	nd = malloc(sizeof(Node));
	nd->ndInfo.id = nodeId;
	strcpy(nd->ndInfo.ipAddr, DEFAULT_IP_ADDR);
	nd->ndInfo.port = DEFAULT_PORT + nodeId;
	nd->keySize = 0;		//initialize the number of keys
	nd->predInfo.id = 0;  	//init predecessor id 
	nd->predInfo.port = 0;  //init predecessor port

	int i = 0;
#if 0	
	// these three lines are for test,
	// TODO: keys should be given by parameters
	// key assigned to node 1
	srand(time(NULL));
	if (nodeId == 1) {
		printf("Keys:");
		for (i = 0; i < 20; i++) {
			nd->key[i] = rand() % 100;
			nd->keySize++;
			printf(" %lu", (unsigned long) nd->key[i]);
		}
		printf("\n");
	}
#endif

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

	//intialize successor list (make them zero) 
	for (i = 0 ; i < (int)pow(2, FT_SIZE); ++i) {
		nd->sList[i].info.id = 0;
		nd->sList[i].info.port = 0; //port 0 means a initial value
	}
	
	if (nodeId >= 2)
		join();

	return 0;
}


#if 0
/**
 * Find the successor of target ID and return the successor ID, IP, and port
 * @param  targetId To be looked for
 * @param  sId      Successor ID of the targetID
 * @param  sipAddr  Successor IP of the targetID
 * @param  sPort    Successor Port of the targetID
 * @return          -1 if error, 2 if found, and 0 if not found
 */
int findSuccessor(uint32_t targetId, uint32_t* sId, char* sIpAddr, uint16_t* sPort) {
	
	//look for id in LOCAL
	int n = closestPrecedingFinger(targetId, sId, sIpAddr, sPort);
	if (n == 2) { // no need to request more
		return 2;
	} else if (n < 0) {
		return -1;
	}

	// Look for targetId in remote
	while (1) {
		sendReqClosestFingerPkt(targetId, *sId, sIpAddr, *sPort);
		uint32_t recvTargetId = 0;
		n = recvResPkt(recvTargetId, sId, sIpAddr, sPort);
		if (n == 2) {
			return 2;
		} else if (n < 0) {
			return -1;
		}
	}

	return 0;
}
#endif

/**
 * Find the successor without looking for it in the local finger table
 * @param  targetId To be looked for
 * @param  sId      Successor ID of the targetID
 * @param  sipAddr  Successor IP of the targetID
 * @param  sPort    Successor Port of the targetID
 * @return          true if found, false if not found
 */
bool findSuccessor(uint32_t targetId, uint32_t* sId, char* sIpAddr, uint16_t* sPort) {
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd == -1) {
		fprintf(stderr, "Socket opening error\n");
		return false;
	}
	
	int pktType = 0;
	while (true) {
		sendReqClosestFingerPkt(sockfd, targetId, *sId, sIpAddr, *sPort);
		pktType = recvResPkt(sockfd, sId, sIpAddr, sPort);
		if (pktType == RES_FIND_CLOSEST_FINGER_FOUND) {
			close(sockfd);
			return true;
		}
	}
	close(sockfd);
	return false;
}

/**
 * [closestPrecedingFinger description]
 * @param  targetId    [description]
 * @param  successorId [description]
 * @param  ipAddr      [description]
 * @param  port        [description]
 * @return             1 if not found, 2 if found
 */
bool closestPrecedingFinger(uint32_t targetId, uint32_t* sId, char* ipAddr, uint16_t* port) {
	if (targetId == nd->ndInfo.id)
		return true;

	int i = 0;
	int ftSize = nd->ftSize;
	struct NodeInfo sNodeInfo;
	for (i = ftSize-1; i >= 0; --i) {
		if (nd->ft[i].sInfo.port != 0 && nd->ft[i].start <= targetId) {
			sNodeInfo = nd->ft[i].sInfo;
			*sId = sNodeInfo.id;
			strcpy(ipAddr, sNodeInfo.ipAddr);
			*port = sNodeInfo.port;
			// No successors between targetId 
			// and the next sucessor
			if (targetId <= sNodeInfo.id || sNodeInfo.id <= nd->ndInfo.id)
				return true;
			
			return false;
		}
	}
	
	for (i = ftSize-1; i >= 0; --i) {
		if (nd->ft[i].sInfo.port != 0) {
			struct NodeInfo ndInfo = nd->ft[i].sInfo;
			*sId = ndInfo.id;
			strcpy(ipAddr, ndInfo.ipAddr);
			*port = ndInfo.port;

			return false;
		}
	}

	return false;
}

/**
 * Join the network
 * @return [description]
 */
void join() {
	while (true) {
		char ipAddr[IPADDR_SIZE];
		strcpy(ipAddr, DEFAULT_IP_ADDR);
		uint32_t sId = DEFAULT_NODE_ID;
		uint32_t targetId = nd->ndInfo.id; // targetID is itself when join
		uint16_t port = DEFAULT_PORT + DEFAULT_NODE_ID;

		uint32_t predId = 0;
		uint16_t predPort = 0;
		char predIpAddr[IPADDR_SIZE];
		
		if (findSuccessor(targetId, &sId, ipAddr, &port)) {
			askSuccForPred(sId, ipAddr, port, &predId, predIpAddr, &predPort);

			fprintf(stderr, "[Join] Found SID %lu, PID %lu \n",
							(unsigned long)sId, (unsigned long)predId);
			
			if (predId > nd->ndInfo.id) {
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

void leave() {
#if 0
	uint32_t sId = nd->ft[0].sInfo.id;
	char sIpAddr[IPADDR_SIZE];
	strcpy(sIpAddr, nd->ft[0].sInfo.ipAddr);
	uint16_t sPort = nd->ft[0].sInfo.port;
	
	//transfer keys to the successor
	int i = 1;
	while (transferKeys(sId, sIpAddr, sPort, nd->key, nd->keySize) < 0) {
		sId = nd->ft[i].sInfo.id;
		strcpy(sIpAddr, nd->ft[i].sInfo.ipAddr);
		sPort = nd->ft[i++].sInfo.port;
		if (i >= nd->ftSize) {
			abort(); //it has no successors 
		}
	}
#endif
	//notify the successor of the predecessor 
}

/**
 * Stablize the node
 */
void stabilize() {
	uint32_t sId = nd->ft[0].sInfo.id;
	uint16_t sPort = nd->ft[0].sInfo.port;
	char sIpAddr[IPADDR_SIZE];
	strcpy(sIpAddr, nd->ft[0].sInfo.ipAddr);
	
	if (sId == 0 || sPort == 0) {
		return;
	}

	// To check to see if the successor fails
	if (!checkAlive(sIpAddr, sPort)) {
		int i = 0;
		for (i = 1; i < (int)pow(2, FT_SIZE); ++i) {
			if (nd->sList[i].sInfo.id <= 0)
				return;
			sId = nd->sList[i].sInfo.id;
			sPort = nd->sList[i].sInfo.port;
			strcpy(sIpAddr, nd->sList[i].sInfo.ipAddr);
			if (checkAlive(sIpAddr, sPort))
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
	
	askSuccForPred(sId, sIpAddr, sPort, &predId, predIpAddr, &predPort);
	
	fprintf(stderr, "[Stabilzing] PredID1: %lu PredPort: %lu SID: %lu\n", (unsigned long) predId, (unsigned long) predPort, (unsigned long) sId);
	if (nd->ndInfo.id != predId) {
		//this node is not just joining & connect
		if (nd->predInfo.port != 0 && checkAlive(predIpAddr,predPort)){ 
			
			fprintf(stderr, "[Stabilzing] PredID2: %lu PredPort: %lu\n", (unsigned long) predId, (unsigned long) predPort);

			nd->ft[0].sInfo.id = predId;
			strcpy(nd->ft[0].sInfo.ipAddr, predIpAddr);
			nd->ft[0].sInfo.port = predPort;
		}

		notify(nd->ndInfo);

#if 0
		/* keys transfer */
		// ask the successor's keys
		uint32_t id = nd->ndInfo.id;
		uint32_t keys[NUM_KEYS];
		int keySize = 0;

		if (sId != nd->ndInfo.id) {
			n = askSuccForKeys(id, sId, sIpAddr, sPort, keys, &keySize);
			int i = 0;
			int size = nd->keySize;
			for (i = 0; i < keySize; ++i) {
				nd->key[size++] = keys[i];
			}
			nd->keySize += keySize;
		}
		// sort key array
		qsort(nd->key, nd->keySize, sizeof(int), cmpfunc);
#endif
	}

	
	if (!(nd->ft[0].sInfo.id == 0 || nd->ft[0].sInfo.port == 0 ||
			nd->ft[0].sInfo.id == nd->ndInfo.id ||
			nd->ft[0].sInfo.port == nd->ndInfo.port || nd->predInfo.id == 0)) {
		
		buildSuccessorList();
		printSuccList();
		fixFingers();
	}

	printDebug();
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
		fprintf(stderr, "%i %s %lu %s %lu\n", __LINE__, __FILE__, (unsigned long) sId, sIpAddr, (unsigned long) sPort);
		ssId = 0; memset(ssIpAddr,0,15); ssPort = 0;
		if (checkAlive(sIpAddr, sPort)) {
			askSuccForSucc(sId, sIpAddr, sPort, &ssId, ssIpAddr, &ssPort);
		} else {
			sId = nd->sList[i-1].info.id;
			sPort = nd->sList[i-1].info.port;
			strcpy(sIpAddr, nd->sList[i-1].info.ipAddr);
			fprintf(stderr, "%i %s %lu %s %lu\n", __LINE__, __FILE__, (unsigned long) sId, sIpAddr, (unsigned long) sPort);
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
}

/**
 * A help function to copy the structure NodeInfo
 * @param src [description]
 * @param dst [description]
 */
void cpyNodeInfo(struct NodeInfo* src, struct NodeInfo* dst) {
	dst->id = src->id;
	dst->port = src->port;
	strcpy(dst->ipAddr, src->ipAddr);
}

#if 0
/**
 * Ask the successor for keys
 * @param  id      [description]
 * @param  sId     [description]
 * @param  sIpAddr [description]
 * @param  sPort   [description]
 * @param  keys    the keys returned
 * @param  keySize     the number of keys returned
 * @return         [description]
 */
void askSuccForKeys(uint32_t id, uint32_t sId, char* sIpAddr, 
					uint16_t sPort, uint32_t keys[], int* keySize) {
	sendReqSuccForKeyPkt(id, sId, sIpAddr, sPort);
	recvKeyTransPkt(keys, keySize);
}
#endif

/**
 * Ask the successor for the successor's successor
 * @param  sId      [description]
 * @param  sIpAddr  [description]
 * @param  sPort    [description]
 * @param  ssId     [description]
 * @param  ssIpAddr [description]
 * @param  ssPort   [description]
 * @return          [description]
 */
void askSuccForSucc(uint32_t sId, char* sIpAddr, uint16_t sPort,
					uint32_t* ssId, char* ssIpAddr, uint16_t* ssPort) {
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd == -1) {
		fprintf(stderr, "Socket opening error\n");
		close(sockfd);
		return;
	}
	sendReqSuccForSuccPkt(sockfd, sId, sIpAddr, sPort);
	recvResPkt(sockfd, ssId, ssIpAddr, ssPort);
	close(sockfd);
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
void askSuccForPred(uint32_t sId, char* sIpAddr, uint16_t sPort,
					uint32_t* predId, char* predIpAddr, uint16_t* predPort) {
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd == -1) {
		fprintf(stderr, "Socket opening error\n");
		close(sockfd);
		return;
	}
	sendReqSuccForPredPkt(sockfd, sId, sIpAddr, sPort);
	recvResPkt(sockfd, predId, predIpAddr, predPort);
	close(sockfd);
}

bool checkAlive(char* ipAddr, uint16_t port) {
	fprintf(stderr, "[checkAlive] start\n");
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	int size = sizeof(char) * 2;
	char* buf = malloc(size);
	int  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))) {
		fprintf(stderr, "Timeout setting error\n");
		return false;
	}
	sendReqAlivePkt(sockfd, ipAddr, port);
	if (recvfrom(sockfd, buf, size, 0, NULL, NULL) < 0) {
		fprintf(stderr, "TImeout reached. \n");
		close(sockfd);
		return false;
	}
	close(sockfd);
	free(buf);
	fprintf(stderr, "[checkAlive] end\n");
	return true;
}

/**
 * Notify the successor of this node as its predecessor
 * @param  pNodeInfo to be a predecessor of its successor 
 * @return           [description]
 */
void notify(struct NodeInfo pNodeInfo) {
	pthread_mutex_lock(&lock);
	uint32_t sId = nd->ft[0].sInfo.id;
	uint16_t sPort = nd->ft[0].sInfo.port;
	char sIpAddr[15];
	strcpy(sIpAddr, nd->ft[0].sInfo.ipAddr);
	uint32_t pId = nd->predInfo.id;

	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd == -1) {
		fprintf(stderr, "Socket opening error\n");
		close(sockfd);
		return;
	}

	if ((pId == 0) || nd->ndInfo.id == pNodeInfo.id) {
		fprintf(stderr, "[Notify] targetId %lu would be changed to %lu\n", (unsigned long)sId, (unsigned long)nd->ndInfo.id);
		sendNotifyPkt(sockfd, sId, sIpAddr, sPort, nd->ndInfo.id, nd->ndInfo.ipAddr, nd->ndInfo.port);
	}

	close(sockfd);
	pthread_mutex_unlock(&lock);
}

/**
 * Fix the finger table
 */
void fixFingers() {
/* OLD VERSION: retrieving via sockets*/
#if 1
	int i = 0;
	uint32_t sId = 0;
	uint16_t sPort = 0;
	char sIpAddr[15];
	uint32_t targetId = 0;

	for (i = 1; i < nd->ftSize; ++i) {
		targetId = nd->ft[i].start;
		if ((targetId <= nd->ft[i-1].sInfo.id) || nd->ft[i-1].sInfo.id == 1){
			nd->ft[i].sInfo.id = nd->ft[i-1].sInfo.id;
			strcpy(nd->ft[i].sInfo.ipAddr, nd->ft[i-1].sInfo.ipAddr);
			nd->ft[i].sInfo.port = nd->ft[i-1].sInfo.port;
		} else {
			// ask its sucessor for the targer ID
			sId = nd->ft[0].sInfo.id;
			strcpy(sIpAddr, nd->ft[0].sInfo.ipAddr);
			sPort = nd->ft[0].sInfo.port;
			if(findSuccessor(targetId, &sId, sIpAddr, &sPort)) {
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

/**
 * Get the successor
 * @param id     [description]
 * @param ipAddr [description]
 * @param port   [description]
 */
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

	/* TODO: 
	* Make it sure that the keys transferred
	* because keys are removed here, but not transfer to predecessor
	*/
	// remove keys from list
	int k = 0;
	for (i = j; i < size; ++i) {
		nd->key[k++] = nd->key[i];
	} 
	
	nd->keySize = size - j;
	*num = j;
}

#if 0
/**
 * Transfer all keys to another node
 * @param id     Node ID
 * @param ipAddr Node IP Addr
 * @param port   Node Port
 * @param keys   Keys to be transferred
 * @param num    The number of keys
 * @return       if less than 0, fail. If 0, successful
 */
int transferKeys(uint32_t id, char* ipAddr, uint16_t port, uint32_t keys[], int keySize) {
	return sendKeyTransPkt(id, ipAddr, port, keys, keySize);
}
#endif 

/**
 * Set keys being transfered by a node leaving
 * @param keys    Keys
 * @param keySize The size of keys
 */
void setKeys(uint32_t keys[], int keySize) {
	int i = 0;
	int size = nd->keySize;
	for (i = 0; i < keySize; ++i) {
		nd->key[size++] = keys[i];
	}
	nd->keySize += keySize;
	// sort key array
	qsort(nd->key, nd->keySize, sizeof(int), cmpfunc);
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
	fprintf(stderr, "ModifyPred: %lu\n", (unsigned long) id);
	pthread_mutex_unlock(&lock);
}

void printDebug() {
	fprintf(stderr, "Predecessor ID: %lu, Predecessor Port: %lu\n", 
		(unsigned long) nd->predInfo.id, 
		(unsigned long) nd->predInfo.port);
	fprintf(stderr, "Successor ID: %lu, Successor Port: %lu\n", 
		(unsigned long) nd->ft[0].sInfo.id, 
		(unsigned long) nd->ft[0].sInfo.port);
	/*int i = 0;
	fprintf(stderr, "Keys:");
	for (i = 0; i < nd->keySize; ++i) {
		fprintf(stderr, " %lu", (unsigned long)nd->key[i]);
	}
	fprintf(stderr, "\n");
	*/
}

void printFT() {
	int i = 0;
	for (i = 0; i < nd->ftSize; ++i) {
		fprintf(stderr, "Start: %d \t Succ: %lu \t IP: %s \t Port: %lu\n", 
			nd->ft[i].start, 
			(unsigned long)nd->ft[i].sInfo.id,
			nd->ft[i].sInfo.ipAddr,
			(unsigned long)nd->ft[i].sInfo.port);
	}
}

void printSuccList() {
	int i = 0;
	// for (i = 0 ; i < (int)pow(2, FT_SIZE); ++i) {
	fprintf(stderr, "Successors: ");
	for (i = 0; i < 100; ++i) {
		// printf("ID: %lu -> SID: %lu \n", 
		// 	(unsigned long)nd->sList[i].info.id,
		// 	(unsigned long)nd->sList[i].sInfo.id);

		if ((unsigned long)nd->sList[i].sInfo.id == 0)
			break;
		fprintf(stderr, "%lu ", (unsigned long)nd->sList[i].sInfo.id);
	}
	fprintf(stderr, "\n");
}