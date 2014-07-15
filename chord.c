/**@file chord.c
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#include "chord.h"
#include "node.h"

/**
 * Initialize Node
 * @return [description]
 */
int initChordNode(uint32_t nodeId) {
	nd = malloc(sizeof(Node));
	nd->ndInfo.id = nodeId;
	strcpy(nd->ndInfo.ipAddr, "127.0.0.1");
	nd->ndInfo.port = DEFAULT_PORT + nodeId;
	nd->keySize = 0; 		//initialize the number of keys
	nd->predInfo.port = 0;
	
	//init ft
	int i = 0;
	for (i = 0; i < FT_SIZE; ++i) {
		int p = (int)pow(2, i);
		if ((nd->ndInfo.id + p) >= (int)pow(2,FT_SIZE))
			break;
		nd->ft[i].start = (nd->ndInfo.id + p) % (int)pow(2,FT_SIZE);
		nd->ft[i].successorInfo.id = 0;
		nd->ft[i].successorInfo.port = DEFAULT_PORT;
		strcpy(nd->ft[i].successorInfo.ipAddr, nd->ndInfo.ipAddr);
		nd->ftSize++;
	}

	if (nodeId != 0)
		join();
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
	
//	 if (targetId == nd->ndInfo.id)
//		 return 0;
	//look for id in LOCAL
	int n = closestPrecedingFinger(targetId, successorId, ipAddr, port);
	if (n == 2) { // no need to request more
		return 2;
	}

	// Look for targetId in remote
	while (1) {
		sendReqPkt(targetId, *successorId, ipAddr, *port);
		uint32_t recvTargetId = 0;
		n = recvResPkt(recvTargetId, successorId, ipAddr, port);
printf("dddddd %lu\n", (unsigned long) successorId);
		if (n == 2) {
			return 2;
		}
	}

	return 0;
}

int findInitSuccessor(uint32_t targetId, uint32_t* successorId,
						char* ipAddr, uint16_t* port) {
	while (1) {
		sendReqPkt(targetId, *successorId, ipAddr, *port);
		uint32_t recvTargetId = 0;
		int n = recvResPkt(recvTargetId, successorId, ipAddr, port);

		if (n == 2) {
			return 2;
		}
	}

	return 0;
}

int closestPrecedingFinger(uint32_t targetId, uint32_t* successorId, 
								char* ipAddr, uint16_t* port) {

	if (targetId == nd->ndInfo.id)
		return 2;

	int i = 0;
	int ftSize = nd->ftSize;
	for (i = ftSize-1; i >= 0; --i) {
		if(nd->ft[i].start <= targetId) {
			struct NodeInfo sNodeInfo = nd->ft[i].successorInfo;
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
	
	struct NodeInfo ndInfo = nd->ft[ftSize-1].successorInfo;
	*successorId = ndInfo.id;
	strcpy(ipAddr, ndInfo.ipAddr);
	*port = ndInfo.port;

	return 1;
}

int join() {
	uint16_t port = DEFAULT_PORT;
	char ipAddr[15];
	strcpy(ipAddr, "127.0.0.1");
	uint32_t successorId = 0;
	uint32_t targetId = nd->ndInfo.id;

	int n = findInitSuccessor(targetId, &successorId, ipAddr, &port);
	if (n == 2) {
		nd->ft[0].successorInfo.id = successorId;
		strcpy(nd->ft[0].successorInfo.ipAddr, ipAddr);
		nd->ft[0].successorInfo.port = port;

		int i = 0;
		for (i = 0; i < nd->ftSize; ++i) {
			if (nd->ft[i].start <= successorId) {
				nd->ft[i].successorInfo.id = successorId;
				strcpy(nd->ft[i].successorInfo.ipAddr, ipAddr);
				nd->ft[i].successorInfo.port = port;
			}
		}
	}

	return 0;
}

void stabilize() {

	printDebug();

	uint32_t sId = nd->ft[0].successorInfo.id;
	uint16_t sPort = nd->ft[0].successorInfo.port;
	char sIpAddr[15];
	strcpy(sIpAddr, nd->ft[0].successorInfo.ipAddr);
	
	//ask succesor for its predecessor
	uint32_t predId = 0;
	uint16_t predPort = 0;
	char predIpAddr[15];
	
	askSuccforPred(sId, sIpAddr, sPort, &predId, predIpAddr, &predPort);

	if (nd->ndInfo.id != predId) {
		if (nd->predInfo.port != 0) { //this node is not just joining.
			nd->ft[0].successorInfo.id = predId;
			strcpy(nd->ft[0].successorInfo.ipAddr, predIpAddr);
			nd->ft[0].successorInfo.port = predPort;
		}
		notify(nd->ndInfo);
	}
}


void askSuccforPred(uint32_t sId, char* sIpAddr, uint16_t sPort,
					uint32_t* predId, char* predIpAddr, uint16_t* predPort){
	// if (nd->ndInfo.id == sId)
		// return;
	sendAskPkt(sId, sIpAddr, sPort, predId, predIpAddr, predPort);
	recvResPkt(sId, predId, predIpAddr, predPort);
}

int notify(struct NodeInfo pNodeInfo) {

	uint32_t sId = nd->ft[0].successorInfo.id;
	uint16_t sPort = nd->ft[0].successorInfo.port;
	char sIpAddr[15];
	strcpy(sIpAddr, nd->ft[0].successorInfo.ipAddr);

	if ((nd->predInfo.port == 0) || nd->ndInfo.id == pNodeInfo.id) { 
		sendNotifyPkt(sId, sIpAddr, sPort, 
						nd->ndInfo.id, nd->ndInfo.ipAddr, nd->ndInfo.port);
	}
}

void fixFingers() {

	int i = 0;
	uint32_t successorId = 0;
	uint16_t port = DEFAULT_PORT;
	char ipAddr[15];
	strcpy(ipAddr, "127.0.0.1");
	uint32_t targetId = 0;
	
	for (i = 1; i < nd->ftSize; ++i) {
		targetId = nd->ft[i].start;
		if (targetId <= nd->ft[i-1].successorInfo.id) {
			nd->ft[i].successorInfo.id = nd->ft[i-1].successorInfo.id;
			strcpy(nd->ft[i].successorInfo.ipAddr, nd->ft[i-1].successorInfo.ipAddr);
			nd->ft[i].successorInfo.port = nd->ft[i-1].successorInfo.port;
		} else{
			int n = findInitSuccessor(targetId, &successorId, ipAddr, &port);
			if (n == 2) {
				nd->ft[i].successorInfo.id = successorId;
				strcpy(nd->ft[i].successorInfo.ipAddr, ipAddr);
				nd->ft[i].successorInfo.port = port;
			}
		}
	}

	printFT();
}

void getPredecesor(uint32_t* id, char* ipAddr, uint16_t* port) {
	*id = nd->predInfo.id;
	strcpy(ipAddr, nd->predInfo.ipAddr);
	*port = nd->predInfo.port;
}

void modifyPred(uint32_t id, char* ipAddr, uint16_t port) {
	nd->predInfo.id = id;
	nd->predInfo.port = port;
	strcpy(nd->predInfo.ipAddr, ipAddr);
}

void printDebug() {
	printf("Successor ID: %lu, Successor Port: %lu\n", 
		(unsigned long) nd->ft[0].successorInfo.id, 
		(unsigned long) nd->ft[0].successorInfo.port);
	printf("Predecessor ID: %lu, Predecessor Port: %lu\n", 
		(unsigned long) nd->predInfo.id, 
		(unsigned long) nd->predInfo.port);
}

void printFT() {
	int i = 0;
	for (i = 0; i < nd->ftSize; ++i) {
		printf("Start: %d \t Succ: %lu \t IP: %s \t Port: %lu\n", 
			nd->ft[i].start, 
			(unsigned long)nd->ft[i].successorInfo.id,
			nd->ft[i].successorInfo.ipAddr,
			(unsigned long)nd->ft[i].successorInfo.port);
	}
}
