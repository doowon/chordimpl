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
void initChord(mpz_t id, FILE* keysfp, uint16_t port) {
	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("mutex init failed\n");
		abort();
	}

	//initalize max and min mpz_t
	unsigned char maxValue[SHA_DIGEST_LENGTH] = {[0 ... SHA_DIGEST_LENGTH-1] = 0xFF};
	mpz_init(max);
	mpz_import(max, SHA_DIGEST_LENGTH, 1, sizeof(maxValue[0]), 1, 0, maxValue);
	mpz_init(min);

	mpz_init(tmp);
	mpz_set_str(tmp, "7c3c1bbda07013241250d5b6154b9de7838989", 16);
	mpz_init(tmp2);
	mpz_set_str(tmp2, "fe762cfc3a765d77eef6c574d2379623e1981fc0", 16);


	nd = malloc(sizeof(Node));
	mpz_init(nd->ndInfo.id);

#if 0
	//big endian for port to buf[]
	char buf[2];
	buf[0] = (port >> 8) & 0xff;
	buf[1] = port & 0xff;
	char hashStr[SHA_DIGEST_LENGTH*2+1];
	hashSHA1(buf, hashStr);
	
	fprintf(stderr, "%s %d ID: %s\n", __FILE__, __LINE__, hashStr);


	if (mpz_set_str(nd->ndInfo.id, hashStr, 16) < 0) {
		abort();
	}
#endif

	mpz_set(nd->ndInfo.id, id);
	
	strcpy(nd->ndInfo.ipAddr, DEFAULT_IP_ADDR);
	nd->ndInfo.port = port;
	mpz_init(nd->predInfo.id);
	nd->predInfo.port = 0;  //init predecessor port

	//init the finger table
	mpz_t tmp; mpz_t n;
	mpz_init(tmp); mpz_init(n);
	int i = 0;
	for (i = 0; i < FT_SIZE; ++i) { 
		mpz_ui_pow_ui(tmp, 2, i);
		mpz_add(n, nd->ndInfo.id, tmp);
		mpz_mod(nd->ft[i].start, n, max);

		//initailize all successors in ft
		mpz_set(nd->ft[i].sInfo.id, nd->ndInfo.id);
		nd->ft[i].sInfo.port = port;
		strcpy(nd->ft[i].sInfo.ipAddr, nd->ndInfo.ipAddr);
		nd->ftSize++;
		
	}
	mpz_clear(tmp); mpz_clear(n);

	//read random key values from file
	nd->keySize = 0; //initialize the number of keys
	if (keysfp != NULL) {
		ssize_t read; size_t len;
		char* line = NULL;
		while ((read = getline(&line, &len, keysfp)) != -1) {
			mpz_init(nd->keyData[nd->keySize].key);
			if (mpz_set_str(nd->keyData[nd->keySize].key, line, 16) < 0) {
				fprintf(stderr, "%s %d %s \n", __FILE__, __LINE__, "can't assign hex to mpz_t");
				abort();
			}
			nd->keySize++;
		}
		fclose(keysfp);
	}
	//intialize successor list (make them zero) 
	for (i = 0 ; i < SLIST_SIZE; ++i) {
		mpz_init(nd->sList[i].info.id);
		nd->sList[i].info.port = 0; //port 0 means a initial value
	}
}

/**
 * Find the successor without looking for it in the local finger table
 * @param  targetId To be looked for
 * @param  sId      Successor ID of the targetID
 * @param  sipAddr  Successor IP of the targetID
 * @param  sPort    Successor Port of the targetID
 * @return          true if found, false if not found
 */
bool findSuccessor(mpz_t targetId, mpz_t sId, char* sIpAddr, uint16_t* sPort) {
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd == -1) {
		fprintf(stderr, "Socket opening error\n");
		return false;
	}
	
	int pktType = 0;
	// char* str = NULL;
	while (true) {
		sendReqClosestFingerPkt(sockfd, targetId, sIpAddr, *sPort);
		pktType = recvResPkt(sockfd, sId, sIpAddr, sPort);

// if (mpz_cmp(targetId, tmp) == 0) {
// 	str = mpz_get_str(NULL, 16, sId);
// 	fprintf(stdout, "sId: %s %d\n", str, *sPort);
// 	free(str);
// }
		if (pktType == RES_FIND_CLOSEST_FINGER_FOUND) {
			close(sockfd);

// if (mpz_cmp(targetId, tmp) == 0) {
// 	str = mpz_get_str(NULL, 16, sId);
// 	fprintf(stdout, "FOUND sId: %s %d\n", str, *sPort);
// 	free(str);
// }
			return true;
		}
	}
	// freeStr(str);
	close(sockfd);
	return false;
}

uint32_t sim_findSuccessor(mpz_t targetId, mpz_t sId, char* sIpAddr, uint16_t* sPort) {
	uint32_t pathLength = 0;
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd == -1) {
		fprintf(stderr, "Socket opening error\n");
		return -1;
	}
	
	int pktType = 0;
	while (true) {
		pathLength++;
		sendReqClosestFingerPkt(sockfd, targetId, sIpAddr, *sPort);
		pktType = recvResPkt(sockfd, sId, sIpAddr, sPort);
		if (pktType == RES_FIND_CLOSEST_FINGER_FOUND) {
			close(sockfd);
			return pathLength;
		}
	}
	close(sockfd);
	return -1;
}

/**
 * [closestPrecedingFinger description]
 * @param  targetId    [description]
 * @param  successorId [description]
 * @param  ipAddr      [description]
 * @param  port        [description]
 * @return             1 if not found, 2 if found
 */
bool closestPrecedingFinger(mpz_t targetId, mpz_t sId, char* ipAddr, uint16_t* port) {
	if (mpz_cmp(targetId, nd->ndInfo.id) == 0) {
		mpz_set(sId, nd->ft[0].sInfo.id);
		strcpy(ipAddr, nd->ft[0].sInfo.ipAddr);
		*port = nd->ft[0].sInfo.port;	
		return true;
	} else if (between(targetId, nd->ndInfo.id, nd->ft[0].sInfo.id, max, min)){
		mpz_set(sId, nd->ft[0].sInfo.id);
		strcpy(ipAddr, nd->ft[0].sInfo.ipAddr);
		*port = nd->ft[0].sInfo.port;
		return true;
	}

	int i = 0;
	for (i = nd->ftSize-1; i > 0; --i) {
		if (between(nd->ft[i].start, nd->ndInfo.id, targetId, max, min)) {
			mpz_set(sId, nd->ft[i].sInfo.id);
			strcpy(ipAddr, nd->ft[i].sInfo.ipAddr);
			*port = nd->ft[i].sInfo.port;

// if (mpz_cmp(nd->ndInfo.id, tmp2) == 0) {
// 	char* str; char* str2;
// 	str = mpz_get_str(NULL, 16, nd->ft[i].start);
// 	str2 = mpz_get_str(NULL, 16, targetId);
// 	fprintf(stdout, "target: %s\t", str2);
// 	fprintf(stdout, " closest: %s %d\n", str, *port);
// 	free(str); free(str2);
// }
			// if (between(targetId, nd->ft[i].start, sId)) {
			// if (mpz_cmp(targetId, sId) <= 0 
			// 	|| (mpz_cmp(targetId, sId) > 0 && mpz_cmp(nd->ft[i].start, sId) > 0)) {
			// 	return true;
			// }
			if (mpz_cmp(sId, nd->ndInfo.id) == 0) {
				return true;
			} else if(between(targetId, nd->ft[i].start, sId, max, min)) {
				return true;
			}
			return false;
		}
	}

	mpz_set(sId, nd->ft[0].sInfo.id);
	strcpy(ipAddr, nd->ft[0].sInfo.ipAddr);
	*port = nd->ft[0].sInfo.port;
	return true;
}


/**
 * Join the network
 * @return [description]
 */
void join() {
// fprintf(stderr, "[Join]----\n");
	while (true) {
		char ipAddr[IPADDR_SIZE];
		strcpy(ipAddr, DEFAULT_IP_ADDR);
		mpz_t sId; mpz_init(sId);
		mpz_t targetId; mpz_init(targetId);
		mpz_set(targetId, nd->ndInfo.id); // targetID is itself when join
		uint16_t port = DEFAULT_PORT;

		mpz_t predId; mpz_init(predId);
		uint16_t predPort = 0;
		char predIpAddr[IPADDR_SIZE];

		char* str = NULL; char* str2 = NULL;
		if (findSuccessor(targetId, sId, ipAddr, &port)) {
			askSuccForPred(sId, ipAddr, port, predId, predIpAddr, &predPort);
			str = mpz_get_str(NULL, 16, sId);
			str2 = mpz_get_str(NULL, 16, predId);
			fprintf(stderr, "[Join] Found SID %s, PID %s \n", str, str2);
			
			// if (mpz_cmp(predId, nd->ndInfo.id) > 0) {
			// 	usleep(500 * 1000);
			// 	continue;
			// }

			mpz_set(nd->ft[0].sInfo.id, sId);
			strcpy(nd->ft[0].sInfo.ipAddr, ipAddr);
			nd->ft[0].sInfo.port = port;

			int i = 0;
			for (i = 1; i < nd->ftSize; ++i) {
				if (between(nd->ft[i].start, nd->ndInfo.id, nd->ft[i-1].sInfo.id, max, min)) {
					mpz_set(nd->ft[i].sInfo.id, nd->ft[i-1].sInfo.id);
					strcpy(nd->ft[i].sInfo.ipAddr, nd->ft[i-1].sInfo.ipAddr);
					nd->ft[i].sInfo.port = nd->ft[i-1].sInfo.port;
				} else {
					if(findSuccessor(nd->ft[i].start, sId, ipAddr, &port)){
						mpz_set(nd->ft[i].sInfo.id, sId);
						strcpy(nd->ft[i].sInfo.ipAddr, ipAddr);
						nd->ft[i].sInfo.port = port;
					}
				}
			}
		}
		
		freeStr(str); freeStr(str2);
		mpz_clear(sId); mpz_clear(targetId); mpz_clear(predId);
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
// fprintf(stderr, "[Stablize Start]\n");
	mpz_t sId; mpz_init(sId);
	mpz_set(sId, nd->ft[0].sInfo.id);
	uint16_t sPort = nd->ft[0].sInfo.port;
	char sIpAddr[IPADDR_SIZE];
	strcpy(sIpAddr, nd->ft[0].sInfo.ipAddr);
	
	if (mpz_cmp(sId, min) == 0 || sPort == 0) {
		mpz_clear(sId);
		return;
	}

	// To check to see if the successor fails
	if (!checkAlive(sIpAddr, sPort)) {
		int i = 0;
		for (i = 0; i < SLIST_SIZE; ++i) {
			if (mpz_cmp(nd->sList[i].sInfo.id, min) == 0){
				mpz_clear(sId);
				return;
			}
			mpz_set(sId, nd->sList[i].sInfo.id);
			sPort = nd->sList[i].sInfo.port;
			strcpy(sIpAddr, nd->sList[i].sInfo.ipAddr);
			if (checkAlive(sIpAddr, sPort))
				mpz_set(nd->ft[0].sInfo.id, sId);
				nd->ft[0].sInfo.port = sPort;
				strcpy(nd->ft[0].sInfo.ipAddr, sIpAddr);
				break;
		}
	}

	//ask succesor for its predecessor
	mpz_t predId; mpz_init(predId);
	uint16_t predPort = 0;
	char predIpAddr[IPADDR_SIZE];
	
	askSuccForPred(sId, sIpAddr, sPort, predId, predIpAddr, &predPort);

	// char* str; char* str2;
	// str = mpz_get_str(NULL, 16, sId);
	// str2 = mpz_get_str(NULL, 16, predId);
	// fprintf(stderr, "[Stabilzing] SID: %s 's PredID1: %s PredPort: %lu\n", str, str2, (unsigned long) predPort);

	if (mpz_cmp(nd->ndInfo.id, predId) != 0) {
		//this node is not just joining & connect
		if (nd->predInfo.port != 0 && checkAlive(predIpAddr, predPort)) { 
			// fprintf(stderr, "[Stabilzing2] PredID: %s PredPort: %lu\n", str2, (unsigned long) str);
			mpz_set(nd->ft[0].sInfo.id, predId);
			strcpy(nd->ft[0].sInfo.ipAddr, predIpAddr);
			nd->ft[0].sInfo.port = predPort;
		}

		notify(nd->ndInfo);
	}

	fixFingers();
	buildSuccessorList();
	
	printDebug();
	printSuccList();
	printFT();

	// freeStr(str); freeStr(str2);
	mpz_clear(sId); mpz_clear(predId);
// fprintf(stderr, "[Stablize END]\n");
}

/**
 * Build the successor list
 */
void buildSuccessorList() {
// fprintf(stderr, "[buildSuccessorList Start]\n");
	int i = 0;

	for (i = 0; i < SLIST_SIZE; ++i) {
		mpz_set(nd->sList[i].info.id, min);
		mpz_set(nd->sList[i].sInfo.id, min);
	}

	//TODO: check to see if the successor is alive.
	cpyNodeInfo(&(nd->ndInfo), &(nd->sList[0].info));
	cpyNodeInfo(&(nd->ft[0].sInfo), &(nd->sList[0].sInfo));

	mpz_t sId; mpz_init(sId);
	mpz_set(sId, nd->sList[0].sInfo.id); 		// the successor
	char sIpAddr[IPADDR_SIZE]; 					// the successor
	strcpy(sIpAddr, nd->sList[0].sInfo.ipAddr);
	uint16_t sPort = nd->sList[0].sInfo.port; 	// the successor

	mpz_t ssId; mpz_init(ssId);
	char ssIpAddr[IPADDR_SIZE]; 				// the successor's successor
	uint16_t ssPort = 0; 						// the successor's successor

	if (mpz_cmp(nd->ndInfo.id, sId) == 0) {
		mpz_clear(sId); mpz_clear(ssId);
		return;
	}

	// char* str;
	for (i = 0; i < SLIST_SIZE; ++i) {
		// str = mpz_get_str(NULL, 16, sId);
		// fprintf(stderr, "%i %s %s %s %lu\n", __LINE__, __FILE__, str , sIpAddr, (unsigned long) sPort);
		// mpz_init(ssId);
		memset(ssIpAddr, 0, IPADDR_SIZE); 
		ssPort = 0;
		
		if (checkAlive(sIpAddr, sPort)) {
			askSuccForSucc(sId, sIpAddr, sPort, ssId, ssIpAddr, &ssPort);
		} else {
			mpz_set(sId, nd->sList[i-1].info.id);
			sPort = nd->sList[i-1].info.port;
			strcpy(sIpAddr, nd->sList[i-1].info.ipAddr);
			// str = mpz_get_str(NULL, 16, sId);
			// fprintf(stderr, "%i %s %s %s %lu\n", __LINE__, __FILE__,  str, sIpAddr, (unsigned long) sPort);
			usleep(100 * 1000);
			continue;
		}

		//successor
		mpz_set(nd->sList[i].info.id, sId);
		nd->sList[i].info.port = sPort;
		strcpy(nd->sList[i].info.ipAddr, sIpAddr);

		//sucessor's successor
		mpz_set(nd->sList[i].sInfo.id, ssId);
		nd->sList[i].sInfo.port = ssPort;
		strcpy(nd->sList[i].sInfo.ipAddr, ssIpAddr);

		//ask for data
		/*
		askSuccForKey(nd->sList[i].info.ipAddr, nd->sList[i].info.port+2000,
						nd->keyData[i+1].data,  &nd->keyData[i+1].dataSize);
		*/
		mpz_set(sId, ssId);
		strcpy(sIpAddr, ssIpAddr);
		sPort = ssPort;

		if (mpz_cmp(nd->ndInfo.id, sId) == 0) {
			mpz_clear(sId); mpz_clear(ssId);
			return;
		}
	}
	// freeStr(str);
	mpz_clear(sId); mpz_clear(ssId);
// fprintf(stderr, "[buildSuccessorList End]\n");
}

void askSuccForKey(mpz_t id, char* sIpAddr, uint16_t sPort, unsigned char* data, int* dataSize) {
#if 0
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == -1) {
		fprintf(stderr, "Socket opening error\n");
		close(sockfd);
		return;
	}
	sendReqSuccForKeyPkt(sockfd, id, sIpAddr, sPort);
	int keySize = 0;
	recvResPkt(sockfd, &keySize); //get keySize
	recvResKeyPkt(sockfd, data, dataSize);

	close(sockfd);
#endif
}

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
void askSuccForSucc(mpz_t sId, char* sIpAddr, uint16_t sPort,
					mpz_t ssId, char* ssIpAddr, uint16_t* ssPort) {
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd == -1) {
		fprintf(stderr, "Socket opening error\n");
		close(sockfd);
		return;
	}
	sendReqSuccForSuccPkt(sockfd, sIpAddr, sPort);
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
void askSuccForPred(mpz_t sId, char* sIpAddr, uint16_t sPort,
					mpz_t predId, char* predIpAddr, uint16_t* predPort) {
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd == -1) {
		fprintf(stderr, "Socket opening error\n");
		close(sockfd);
		return;
	}
	sendReqSuccForPredPkt(sockfd, sIpAddr, sPort);
	recvResPkt(sockfd, predId, predIpAddr, predPort);
	close(sockfd);
}

bool checkAlive(char* ipAddr, uint16_t port) {
// fprintf(stderr, "[checkAlive] start\n");
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
	freeStr(buf);
// fprintf(stderr, "[checkAlive] end\n");
	return true;
}

/**
 * Notify the successor of this node as its predecessor
 * @param  pNodeInfo to be a predecessor of its successor 
 * @return           [description]
 */
void notify(struct NodeInfo pNodeInfo) {
	pthread_mutex_lock(&lock);
	mpz_t sId;
	mpz_init(sId);
	mpz_set(sId, nd->ft[0].sInfo.id);
	uint16_t sPort = nd->ft[0].sInfo.port;
	char sIpAddr[IPADDR_SIZE];
	strcpy(sIpAddr, nd->ft[0].sInfo.ipAddr);
	mpz_t pId;
	mpz_init(pId);
	mpz_set(pId, nd->predInfo.id);

	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd == -1) {
		fprintf(stderr, "Socket opening error\n");
		close(sockfd);
		return;
	}

	mpz_t tmp;
	mpz_init(tmp);
	// char* str; char* str2;
	if ((mpz_cmp(pId, tmp) == 0) || mpz_cmp(nd->ndInfo.id, pNodeInfo.id) == 0) {
		// str = mpz_get_str(NULL, 16, sId);
		// str2 = mpz_get_str(NULL, 16, nd->ndInfo.id);
		// fprintf(stderr, "[Notify] sID %s 's successor would be changed to %s\n", str, str2);
		sendNotifyPkt(sockfd, sIpAddr, sPort, nd->ndInfo.id, nd->ndInfo.ipAddr, nd->ndInfo.port);
	}

	// freeStr(str); freeStr(str2);
	mpz_clear(sId); mpz_clear(pId); mpz_clear(tmp);
	close(sockfd);
	pthread_mutex_unlock(&lock);
}

/**
 * Fix the finger table
 */
void fixFingers() {
// fprintf(stderr, "[Fix Fingers Started]-----\n");
	int i = 0;
	mpz_t sId; mpz_init(sId);
	uint16_t sPort = 0;
	char sIpAddr[IPADDR_SIZE];
	mpz_t targetId; mpz_init(targetId);

	for (i = 1; i < nd->ftSize; ++i) {
		mpz_set(targetId, nd->ft[i].start);

// if (mpz_cmp(targetId, tmp) == 0) {
// 	printf("[START]\n");
// }

		if (between(targetId, nd->ft[i-1].start, nd->ft[i-1].sInfo.id, max, min)) {
			mpz_set(nd->ft[i].sInfo.id, nd->ft[i-1].sInfo.id);
			strcpy(nd->ft[i].sInfo.ipAddr, nd->ft[i-1].sInfo.ipAddr);
			nd->ft[i].sInfo.port = nd->ft[i-1].sInfo.port;
		} 
		else {

// if (mpz_cmp(targetId, tmp) == 0) {
// 	char* a = mpz_get_str(NULL, 16, nd->ft[0].sInfo.id);
// 	printf("Node to ask: %s %d\n", a, nd->ft[0].sInfo.port);
// 	free(a);
// }
			// ask its sucessor for the targer ID
			strcpy(sIpAddr, nd->ft[0].sInfo.ipAddr);
			sPort = nd->ft[0].sInfo.port;
			if(findSuccessor(targetId, sId, sIpAddr, &sPort)) {
				mpz_set(nd->ft[i].sInfo.id, sId);
				strcpy(nd->ft[i].sInfo.ipAddr, sIpAddr);
				nd->ft[i].sInfo.port = sPort;
			}
		}
	}

	// printFT();
	mpz_clear(sId); mpz_clear(targetId);
// fprintf(stderr, "[Fix Fingers Ended]-----\n");
}

/**
 * Get its predecessor
 * @param id     predecessor id
 * @param ipAddr predecessor ip address
 * @param port   predecessor port
 */
void getPredecesor(mpz_t id, char* ipAddr, uint16_t* port) {
	mpz_set(id, nd->predInfo.id);
	strcpy(ipAddr, nd->predInfo.ipAddr);
	*port = nd->predInfo.port;
}

/**
 * Get the successor
 * @param id     [description]
 * @param ipAddr [description]
 * @param port   [description]
 */
void getSuccessor(mpz_t id, char* ipAddr, uint16_t* port) {
	mpz_set(id, nd->ft[0].sInfo.id);
	strcpy(ipAddr, nd->ft[0].sInfo.ipAddr);
	*port = nd->ft[0].sInfo.port;
}

#if 0
int getData(unsigned char* data) {
	memcpy(data, nd->keyData[0].data, nd->keyData[0].dataSize);
	return nd->keyData[0].dataSize;
}
#endif

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
* Get keys to be moved to the request node
* @param id the request node ID
* @param keys keys to be moved to the request node
* @param num the number of the returned keys
*/
void getKeys(mpz_t id, mpz_t keys[], int keySize, char* data[]) {
#if 0
	int i = 0; int j = 0;
	int size = nd->keySize;
	for (i = 0; i < size; ++i) {
		if (mpz_cmp(id, nd->keyData[i].key) >= 0 
				&& mpz_cmp(nd->keyData[i].key, nd->ndInfo.id) != 0) {
			mpz_set(keys[j], nd->keyData[i].key);
			data[j] = malloc(DATA_SIZE);
			memcpy(data[j++], nd->keyData[i].data, nd->keyData[i].dataSize); 
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
		mpz_set(nd->keyData[k++].key, nd->keyData[i].key);
	}
	nd->keySize = size - j;
	*keySize = j;
#endif
}

/**
 * Set keys being transfered by a node leaving
 * @param keys    Keys
 * @param keySize The size of keys
 */

void setKeys(mpz_t keys[], int keySize) {
#if 0
	int i = 0;
	int size = nd->keySize;
	for (i = 0; i < keySize; ++i) {
		mps_set(nd->keyData[k++].key, keys[i]);
	}
	nd->keySize += keySize;
	// sort key array
	qsort(nd->keyData, nd->keySize, sizeof(struct KeyData), cmpfunc);
#endif
}

/**
 * Modify its predecessor
 * @param id     predecessor id
 * @param ipAddr predecessor ip address
 * @param port   predecessor port
 */
void modifyPred(mpz_t id, char* ipAddr, uint16_t port) {
	pthread_mutex_lock(&lock);
	mpz_set(nd->predInfo.id, id);
	nd->predInfo.port = port;
	strcpy(nd->predInfo.ipAddr, ipAddr);
	pthread_mutex_unlock(&lock);

	// char* str = mpz_get_str(NULL, 16, id);
	// fprintf(stderr, "ModifyPredID: %s\n", str);
	// freeStr(str);
}

/**
 * A help function to copy the structure NodeInfo
 * @param src [description]
 * @param dst [description]
 */
void cpyNodeInfo(struct NodeInfo* src, struct NodeInfo* dst) {
	assert(src); assert(dst);
	mpz_set(dst->id, src->id);
	dst->port = src->port;
	strcpy(dst->ipAddr, src->ipAddr);
}

void printDebug() {
	char* mdId = mpz_get_str(NULL, 16, nd->ndInfo.id);
	char* mdString = mpz_get_str(NULL, 16, nd->ft[0].sInfo.id);
	char* mdString2 = mpz_get_str(NULL, 16, nd->predInfo.id);
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
	freeStr(mdId); freeStr(mdString); freeStr(mdString2);
}

void printFT() {
	fprintf(stderr, "[FT START]\n");
	char* str;
	char* str2;

	int i = 0;
	for (i = 0; i < nd->ftSize; ++i) {
		str = mpz_get_str(NULL, 16, nd->ft[i].start);
		str2 = mpz_get_str(NULL, 16, nd->ft[i].sInfo.id);
		fprintf(stderr, "Start: %s \t Succ: %s \t IP: %s \t Port: %lu\n", 
			str, str2, nd->ft[i].sInfo.ipAddr,
			(unsigned long)nd->ft[i].sInfo.port);
		freeStr(str); freeStr(str2);
	}
	fprintf(stderr, "[FT END]\n");
}

void printSuccList() {
	int i = 0;
	fprintf(stderr, "Successors: ");
	char* md;
	char* md2;
	for (i = 0; i < SLIST_SIZE; ++i) {
		md = mpz_get_str(NULL, 16, nd->sList[i].sInfo.id);
		md2 = mpz_get_str(NULL, 16, nd->sList[i].info.id);
		fprintf(stderr, "%s (data: %s)-> ", md2, nd->keyData[i+1].data);
		fprintf(stderr, "%s \n", md);
		freeStr(md); freeStr(md2);
	}
}

/*------------------------
For Simulation
--------------------------*/

void sim_pathLength() {
	sleep(10 + pow(2,6));
	mpz_t sId; mpz_init(sId);
	char ipAddr[IPADDR_SIZE];
	strcpy(ipAddr, DEFAULT_IP_ADDR);
	uint16_t port = nd->ft[0].sInfo.port;
	
	int i = 0;
	uint32_t res = 0;
	for (i = 0; i < simSize; ++i) {
		res = 0;
		res = sim_findSuccessor(simKeys[i], sId, ipAddr, &port);
		printf("%lu\n", (unsigned long) res);
		fflush(stdout);
	}
	mpz_clear(sId);
}
