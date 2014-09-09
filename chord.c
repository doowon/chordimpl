/** @file chord.c
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#include "chord.h"
#include "node.h"
#include "util.h"

/**
 * Initialize Chord
* @param  nodeId     Node ID to be assigned 
 * @return            [description]
 */
void initChord(char* data, int dataSize, uint16_t port) {

	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("mutex init failed\n");
		abort();
	}

	nd = malloc(sizeof(Node));
	hashSHA1(data, nd->ndInfo.id);
	memcpy(nd->keyData[0].key, nd->ndInfo.id, SHA_DIGEST_LENGTH);
	memcpy(nd->keyData[0].data, data, dataSize);
	strcpy(nd->ndInfo.ipAddr, DEFAULT_IP_ADDR);
	nd->ndInfo.port = port;
	nd->keySize = 0;		//initialize the number of keys
	memset(nd->predInfo.id, 0, SHA_DIGEST_LENGTH); //init predecessor id 
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
		unsigned char value[SHA_DIGEST_LENGTH];
		memset(value, 0, SHA_DIGEST_LENGTH);
		power2(i, value);
		addValueToHash(nd->ndInfo.id, value, nd->ft[i].start);
		// nd->ft[i].sInfo.port = 0;
		// if (i == 0) {
			memcpy(nd->ft[i].sInfo.id, nd->ndInfo.id, SHA_DIGEST_LENGTH);
			nd->ft[i].sInfo.port = port;
		// }
		strcpy(nd->ft[i].sInfo.ipAddr, nd->ndInfo.ipAddr);
		nd->ftSize++;
	}

	//intialize successor list (make them zero) 
	for (i = 0 ; i < SLIST_SIZE; ++i) {
		// nd->sList[i].info.id = NULL;
		nd->sList[i].info.port = 0; //port 0 means a initial value
	}
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
bool findSuccessor(unsigned char* targetId, unsigned char* sId, char* sIpAddr, 
					uint16_t* sPort) {
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd == -1) {
		fprintf(stderr, "Socket opening error\n");
		return false;
	}
	
	int pktType = 0;
	while (true) {
		sendReqClosestFingerPkt(sockfd, targetId, sIpAddr, *sPort);
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
bool closestPrecedingFinger(unsigned char* targetId, unsigned char* sId, char* ipAddr, uint16_t* port) {
	if (cmpHashValue(targetId, nd->ndInfo.id) == 0) {
		memcpy(sId, nd->ndInfo.id, SHA_DIGEST_LENGTH);
		strcpy(ipAddr, nd->ndInfo.ipAddr);
		*port = nd->ndInfo.port;	
		
		return true;
	}

	int i = 0;
	for (i = nd->ftSize-1; i > 0; --i) {
		if (between(nd->ft[i].start, nd->ndInfo.id, targetId)) {
			memcpy(sId, nd->ft[i].sInfo.id, SHA_DIGEST_LENGTH);
			strcpy(ipAddr, nd->ft[i].sInfo.ipAddr);
			*port = nd->ft[i].sInfo.port;

			if (between(targetId, nd->ft[i].start, sId)) {
				return true;
			}

			return false;
		}

/*	
		if (cmpHashValue(nd->ft[i].start, targetId) <= 0) {
			memcpy(sId, nd->ft[i].sInfo.id, SHA_DIGEST_LENGTH);
			strcpy(ipAddr, nd->ft[i].sInfo.ipAddr);
			*port = nd->ft[i].sInfo.port;
			// No successors between targetId and the next sucessor
			if (cmpHashValue(targetId, nd->ft[i].sInfo.id) <= 0 
				|| cmpHashValue(nd->ft[i].sInfo.id, nd->ndInfo.id) <= 0) {
				return true;
			}
			
			return false;
		}
	}

	for (i = nd->ftSize-1; i >= 0; --i) {
		if (nd->ft[i].sInfo.port != 0) {
			memcpy(sId, nd->ft[i].sInfo.id, SHA_DIGEST_LENGTH);
			strcpy(ipAddr, nd->ft[i].sInfo.ipAddr);
			*port = nd->ft[i].sInfo.port;

			return false;
		}
*/
	}

	memcpy(sId, nd->ft[0].sInfo.id, SHA_DIGEST_LENGTH);
	strcpy(ipAddr, nd->ft[0].sInfo.ipAddr);
	*port = nd->ft[0].sInfo.port;

	return true;
}

bool between(const unsigned char* id, const unsigned char* start, 
				const unsigned char* end) {

	unsigned char max[SHA_DIGEST_LENGTH] = {0xff,};
	unsigned char min[SHA_DIGEST_LENGTH] = {0x00,};

	if (cmpHashValue(start, end) < 0 && cmpHashValue(start, id) <= 0 
		&& cmpHashValue(id, end) <= 0) {
		return true;
	} else if ((cmpHashValue(start, end) > 0) && 
				((cmpHashValue(start, id) <= 0 && cmpHashValue(id, max) <= 0)
				|| (cmpHashValue(min, id) <= 0 && cmpHashValue(id, end) <= 0))) {
		return true;
	} else if (cmpHashValue(start, end) == 0 && cmpHashValue(id, end) == 0) {
		return true;
	}

	return false;
}
/**
 * Join the network
 * @return [description]
 */
void join() {
fprintf(stderr, "[Join]----\n");
	while (true) {
		char ipAddr[IPADDR_SIZE];
		strcpy(ipAddr, DEFAULT_IP_ADDR);
		unsigned char sId[SHA_DIGEST_LENGTH];
		memset(sId, 0, SHA_DIGEST_LENGTH); // default is 0
		unsigned char targetId[SHA_DIGEST_LENGTH];
		memcpy(targetId, nd->ndInfo.id, SHA_DIGEST_LENGTH); // targetID is itself when join
		uint16_t port = DEFAULT_PORT;

		unsigned char predId[SHA_DIGEST_LENGTH];
		uint16_t predPort = 0;
		char predIpAddr[IPADDR_SIZE];

		if (findSuccessor(targetId, sId, ipAddr, &port)) {
			askSuccForPred(sId, ipAddr, port, predId, predIpAddr, &predPort);

			char mdString[SHA_DIGEST_LENGTH*2+1];
			char mdString2[SHA_DIGEST_LENGTH*2+1];
			hashToString(sId, mdString);
			hashToString(predId, mdString2);
			fprintf(stderr, "[Join] Found SID %s, PID %s \n", mdString, mdString2);
			
			// if (cmpHashValue(predId, nd->ndInfo.id) > 0) {
			// 	usleep(500 * 1000);
			// 	continue;
			// }

			memcpy(nd->ft[0].sInfo.id, sId, SHA_DIGEST_LENGTH);
			strcpy(nd->ft[0].sInfo.ipAddr, ipAddr);
			nd->ft[0].sInfo.port = port;

			int i = 0;
			for (i = 0; i < nd->ftSize; ++i) {
				if (cmpHashValue(nd->ft[i].start, sId) <= 0) {
					memcpy(nd->ft[i].sInfo.id, sId, SHA_DIGEST_LENGTH);
					strcpy(nd->ft[i].sInfo.ipAddr, ipAddr);
					nd->ft[i].sInfo.port = port;
				}
			}
		}
		return;
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
	
	unsigned char sId[SHA_DIGEST_LENGTH];
	memcpy(sId, nd->ft[0].sInfo.id, SHA_DIGEST_LENGTH);
	uint16_t sPort = nd->ft[0].sInfo.port;
	char sIpAddr[IPADDR_SIZE];
	strcpy(sIpAddr, nd->ft[0].sInfo.ipAddr);
	
	unsigned char tmp[SHA_DIGEST_LENGTH];
	memset(tmp, 0, SHA_DIGEST_LENGTH);
	if (cmpHashValue(sId, tmp) == 0 || sPort == 0) {
		return;
	}

	// To check to see if the successor fails
	if (!checkAlive(sIpAddr, sPort)) {
		int i = 0;
		for (i = 1; i < SLIST_SIZE; ++i) {
			if (cmpHashValue(nd->sList[i].sInfo.id, tmp) == 0)
				return;
			memcpy(sId, nd->sList[i].sInfo.id, SHA_DIGEST_LENGTH);
			sPort = nd->sList[i].sInfo.port;
			strcpy(sIpAddr, nd->sList[i].sInfo.ipAddr);
			if (checkAlive(sIpAddr, sPort))
				memcpy(nd->ft[0].sInfo.id, sId, SHA_DIGEST_LENGTH);
				nd->ft[0].sInfo.port = sPort;
				strcpy(nd->ft[0].sInfo.ipAddr, sIpAddr);
				break;
		}
	}

	printDebug();

	//ask succesor for its predecessor
	unsigned char predId[SHA_DIGEST_LENGTH];
	uint16_t predPort = 0;
	char predIpAddr[IPADDR_SIZE];
	
	askSuccForPred(sId, sIpAddr, sPort, predId, predIpAddr, &predPort);
	
	char mdString[SHA_DIGEST_LENGTH*2+1];
	char mdString2[SHA_DIGEST_LENGTH*2+1];
	hashToString(sId, mdString);
	hashToString(predId, mdString2);
	fprintf(stderr, "[Stabilzing] PredID1: %s PredPort: %lu SID: %s\n", mdString2, (unsigned long) predPort, mdString);
	if (cmpHashValue(nd->ndInfo.id, predId) != 0) {
		//this node is not just joining & connect
		if (nd->predInfo.port != 0 && checkAlive(predIpAddr, predPort)) { 
			
			fprintf(stderr, "[Stabilzing] PredID2: %s PredPort: %lu\n", mdString2, (unsigned long) predPort);

			memcpy(nd->ft[0].sInfo.id, predId, SHA_DIGEST_LENGTH);
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

	fixFingers();
	printDebug();
}

/**
 * Build the successor list
 */
void buildSuccessorList() {
	int i = 0;
	for (i = 0 ; i < SLIST_SIZE; ++i) {
		memset(nd->sList[i].info.id, 0, SHA_DIGEST_LENGTH);
		nd->sList[i].info.port = 0;
		memset(nd->sList[i].info.ipAddr, 0, IPADDR_SIZE);

		memset(nd->sList[i].sInfo.id, 0, SHA_DIGEST_LENGTH);
		nd->sList[i].sInfo.port = 0;
		memset(nd->sList[i].sInfo.ipAddr, 0, IPADDR_SIZE);

	}

	//TODO: check to see if the successor is alive.
	cpyNodeInfo(&(nd->ndInfo), &(nd->sList[0].info));
	cpyNodeInfo(&(nd->ft[0].sInfo), &(nd->sList[0].sInfo));

	unsigned char sId[SHA_DIGEST_LENGTH];
	memcpy(sId, nd->sList[0].sInfo.id, SHA_DIGEST_LENGTH);// the successor
	char sIpAddr[IPADDR_SIZE]; // the successor
	strcpy(sIpAddr, nd->sList[0].sInfo.ipAddr);
	uint16_t sPort = nd->sList[0].sInfo.port; // the successor

	unsigned char ssId[SHA_DIGEST_LENGTH];
	memset(ssId, 0, SHA_DIGEST_LENGTH); // the successor's successor
	char ssIpAddr[IPADDR_SIZE]; // the successor's successor
	uint16_t ssPort = 0; // the successor's successor

	i = 1;
	char mdString[SHA_DIGEST_LENGTH*2+1];
	while (cmpHashValue(nd->ndInfo.id, sId) != 0) {
		hashToString(sId, mdString);
		fprintf(stderr, "%i %s %s %s %lu\n", __LINE__, __FILE__, mdString , sIpAddr, (unsigned long) sPort);
		memset(ssId, 0, SHA_DIGEST_LENGTH);
		memset(ssIpAddr,0,15); 
		ssPort = 0;
		if (checkAlive(sIpAddr, sPort)) {
			askSuccForSucc(sId, sIpAddr, sPort, ssId, ssIpAddr, &ssPort);
		} else {
			memcpy(sId, nd->sList[i-1].info.id, SHA_DIGEST_LENGTH);
			sPort = nd->sList[i-1].info.port;
			strcpy(sIpAddr, nd->sList[i-1].info.ipAddr);
			hashToString(sId, mdString);
			fprintf(stderr, "%i %s %s %s %lu\n", __LINE__, __FILE__,  sId, sIpAddr, (unsigned long) sPort);
			usleep(100 * 1000);
			continue;
		}
		
		//successor
		memcpy(nd->sList[i].info.id, sId, SHA_DIGEST_LENGTH);
		nd->sList[i].info.port = sPort;
		strcpy(nd->sList[i].info.ipAddr, sIpAddr);

		//sucessor's successor
		memcpy(nd->sList[i].sInfo.id, ssId, SHA_DIGEST_LENGTH);
		nd->sList[i].sInfo.port = ssPort;
		strcpy(nd->sList[i++].sInfo.ipAddr, ssIpAddr);

		memcpy(sId, ssId, SHA_DIGEST_LENGTH);
		strcpy(sIpAddr, ssIpAddr);
		sPort = ssPort;
	}
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
void askSuccForSucc(unsigned char* sId, char* sIpAddr, uint16_t sPort,
					unsigned char* ssId, char* ssIpAddr, uint16_t* ssPort) {
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
void askSuccForPred(unsigned char* sId, char* sIpAddr, uint16_t sPort,
					unsigned char* predId, char* predIpAddr, uint16_t* predPort) {
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd == -1) {
		fprintf(stderr, "Socket opening error\n");
		close(sockfd);
		return;
	}
	sendReqSuccForPredPkt(sockfd, NULL, sIpAddr, sPort);
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
	unsigned char sId[SHA_DIGEST_LENGTH];
	memcpy(sId, nd->ft[0].sInfo.id, SHA_DIGEST_LENGTH);
	uint16_t sPort = nd->ft[0].sInfo.port;
	char sIpAddr[IPADDR_SIZE];
	strcpy(sIpAddr, nd->ft[0].sInfo.ipAddr);
	unsigned char pId[SHA_DIGEST_LENGTH];
	memcpy(pId, nd->predInfo.id, SHA_DIGEST_LENGTH);

	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd == -1) {
		fprintf(stderr, "Socket opening error\n");
		close(sockfd);
		return;
	}

	unsigned char tmp[SHA_DIGEST_LENGTH];
	memset(tmp, 0, SHA_DIGEST_LENGTH);

	if ((cmpHashValue(pId, tmp) == 0) || cmpHashValue(nd->ndInfo.id, pNodeInfo.id) == 0) {
		char mdString[SHA_DIGEST_LENGTH*2+1];
		char mdString2[SHA_DIGEST_LENGTH*2+1];
		hashToString(sId, mdString);
		hashToString(nd->ndInfo.id, mdString2);
		fprintf(stderr, "[Notify] targetId %s would be changed to %s\n", mdString, mdString2);
		sendNotifyPkt(sockfd, sId, sIpAddr, sPort, nd->ndInfo.id, nd->ndInfo.ipAddr, nd->ndInfo.port);
	}

	close(sockfd);
	pthread_mutex_unlock(&lock);
}

/**
 * Fix the finger table
 */
void fixFingers() {
	int i = 0;
	unsigned char sId[SHA_DIGEST_LENGTH];
	uint16_t sPort = 0;
	char sIpAddr[IPADDR_SIZE];
	unsigned char targetId[SHA_DIGEST_LENGTH];
#if 1
	for (i = 1; i < nd->ftSize; ++i) {
		memcpy(targetId, nd->ft[i].start, SHA_DIGEST_LENGTH);
		if (between(targetId, nd->ft[i-1].start, nd->ft[i-1].sInfo.id)) {
			memcpy(nd->ft[i].sInfo.id, nd->ft[i-1].sInfo.id, SHA_DIGEST_LENGTH);
			strcpy(nd->ft[i].sInfo.ipAddr, nd->ft[i-1].sInfo.ipAddr);
			nd->ft[i].sInfo.port = nd->ft[i-1].sInfo.port;
		} else {
			// ask its sucessor for the targer ID
			strcpy(sIpAddr, nd->ft[0].sInfo.ipAddr);
			sPort = nd->ft[0].sInfo.port;
			if(findSuccessor(targetId, sId, sIpAddr, &sPort)) {
				memcpy(nd->ft[i].sInfo.id, sId, SHA_DIGEST_LENGTH);
				strcpy(nd->ft[i].sInfo.ipAddr, sIpAddr);
				nd->ft[i].sInfo.port = sPort;
			}
		}
	}
#endif
#if 0
	for (i = 1; i < nd->ftSize; ++i) {
		memcpy(targetId, nd->ft[i].start, SHA_DIGEST_LENGTH);
		strcpy(sIpAddr, nd->ndInfo.ipAddr);
		sPort = nd->ndInfo.port;
		if(findSuccessor(targetId, sId, sIpAddr, &sPort)) {
			memcpy(nd->ft[i].sInfo.id, sId, SHA_DIGEST_LENGTH);
			strcpy(nd->ft[i].sInfo.ipAddr, sIpAddr);
			nd->ft[i].sInfo.port = sPort;
		}
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
void getPredecesor(unsigned char* id, char* ipAddr, uint16_t* port) {
	memcpy(id, nd->predInfo.id, SHA_DIGEST_LENGTH);
	strcpy(ipAddr, nd->predInfo.ipAddr);
	*port = nd->predInfo.port;
}

/**
 * Get the successor
 * @param id     [description]
 * @param ipAddr [description]
 * @param port   [description]
 */
void getSuccessor(unsigned char* id, char* ipAddr, uint16_t* port) {
	memcpy(id, nd->ft[0].sInfo.id, SHA_DIGEST_LENGTH);
	strcpy(ipAddr, nd->ft[0].sInfo.ipAddr);
	*port = nd->ft[0].sInfo.port;
}

/**
 * Get keys to be moved to the request node
 * @param id   the request node ID
 * @param keys keys to be moved to the request node
 * @param num  the number of the returned keys 
 */

void getKeys(unsigned char* id, uint32_t keys[], int* num) {
#if 0
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
#endif
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
#if 0
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
#endif

/**
 * Modify its predecessor
 * @param id     predecessor id
 * @param ipAddr predecessor ip address
 * @param port   predecessor port
 */
void modifyPred(unsigned char* id, char* ipAddr, uint16_t port) {
	pthread_mutex_lock(&lock);
	memcpy(nd->predInfo.id, id, SHA_DIGEST_LENGTH);
	nd->predInfo.port = port;
	strcpy(nd->predInfo.ipAddr, ipAddr);
	pthread_mutex_unlock(&lock);

	char md[SHA_DIGEST_LENGTH*2+1];
	hashToString(id, md);
	fprintf(stderr, "ModifyPredID: %s\n", md);
}

/**
 * A help function to copy the structure NodeInfo
 * @param src [description]
 * @param dst [description]
 */
void cpyNodeInfo(struct NodeInfo* src, struct NodeInfo* dst) {
	memcpy(dst->id, src->id, SHA_DIGEST_LENGTH);
	dst->port = src->port;
	strcpy(dst->ipAddr, src->ipAddr);
}

void printDebug() {
	char mdId[SHA_DIGEST_LENGTH*2+1];
	char mdString[SHA_DIGEST_LENGTH*2+1];
	char mdString2[SHA_DIGEST_LENGTH*2+1];
	hashToString(nd->ft[0].sInfo.id, mdString);
	hashToString(nd->predInfo.id, mdString2);
	hashToString(nd->ndInfo.id, mdId);
	fprintf(stderr, "ID: %s\n", mdId);
	fprintf(stderr, "Predecessor ID: %s, Predecessor Port: %lu\n", 
		mdString2, (unsigned long) nd->predInfo.port);
	fprintf(stderr, "Successor ID: %s, Successor Port: %lu\n", 
		mdString, (unsigned long) nd->ft[0].sInfo.port);
	
	/*int i = 0;
	fprintf(stderr, "Keys:");
	for (i = 0; i < nd->keySize; ++i) {
		fprintf(stderr, " %lu", (unsigned long)nd->key[i]);
	}
	fprintf(stderr, "\n");
	*/
}

void printFT() {
	fprintf(stderr, "[FT START]\n");
	char mdString[SHA_DIGEST_LENGTH*2+1];
	char mdString2[SHA_DIGEST_LENGTH*2+1];

	int i = 0;
	for (i = 0; i < nd->ftSize; ++i) {
		hashToString(nd->ft[i].start, mdString);
		hashToString(nd->ft[i].sInfo.id, mdString2);
		fprintf(stderr, "Start: %s \t Succ: %s \t IP: %s \t Port: %lu\n", 
			mdString, mdString2, nd->ft[i].sInfo.ipAddr,
			(unsigned long)nd->ft[i].sInfo.port);
	}
	fprintf(stderr, "[FT END]\n");
}

void printSuccList() {
	int i = 0;
	// for (i = 0 ; i < (int)pow(2, FT_SIZE); ++i) {
	fprintf(stderr, "Successors: ");
	for (i = 0; i < 100; ++i) {
		if ((unsigned long)nd->sList[i].sInfo.id == 0)
			break;
		fprintf(stderr, "%lu ", (unsigned long)nd->sList[i].sInfo.id);
	}
	fprintf(stderr, "\n");
}