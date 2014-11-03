/** @file node.c
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#include "node.h"
#include "chord.h"
#include "util.h"

/**
 * Init Node
 * @param  nodeId     [description]
 * @return            [description]
 */
int initNode(char* idFile, char* keysFile, char* lkupFile, uint16_t port, int fTime, bool menu) {
	// For Simulation start
	ssize_t read; size_t len;
	char* line = NULL;
	simSize = 0;
	mpz_t id; mpz_init(id);

	FILE* fp = fopen(lkupFile, "r+");
	if (fp == NULL) {
		fprintf(stderr, "%d Can't open data file, %s\n", __LINE__, lkupFile);
	} else {
		while ((read = getline(&line, &len, fp)) != -1) {
			mpz_init(simKeys[simSize]);
			if (mpz_set_str(simKeys[simSize++], line, 16) < 0) {
				fprintf(stderr, "%s %d %s \n", __FILE__, __LINE__, "can't assign hex to mpz_t");
				abort();
			}
		}
		fclose(fp);
	}

	FILE* idfp = fopen(idFile, "r+");
	if (idfp == NULL) {
		fprintf(stderr, "%d Can't open data file, %s\n", __LINE__, idFile);
		abort();
	} else {
		while ((read = getline(&line, &len, idfp)) != -1) {
			if (mpz_set_str(id, line, 16) < 0) {
				fprintf(stderr, "%s %d %s \n", __FILE__, __LINE__, "can't assign hex to mpz_t");
				abort();
			}
		}
		fclose(idfp);
	}

	FILE* keysfp = fopen(keysFile, "r+");
	if (keysfp == NULL) {
		fprintf(stderr, "%d Can't open data file, %s\n", __LINE__, keysFile);
	} 
	// Simulation end

	initChord(id, keysfp, port);
	fprintf(stderr, "init done\n");
	fflush(stderr);
	initServerSocket(port);

	//create a thread of server socket 
	int err = pthread_create(&(tid[0]), NULL, (void*)&listenServerUDPSocket, NULL);
	if (err != 0) {
		fprintf(stderr, "can't create a pthread of a server socket\n");
		abort();
	}

	// err = pthread_create(&(tid[1]), NULL, (void*)&listenServerTCPSocket, NULL);
	// if (err != 0) {
	// 	fprintf(stderr, "can't create a pthread of a server socket\n");
	// 	abort();
	// }
	
	//create a thread of looping stablizing
	err = pthread_create(&(tid[1]), NULL, (void*)&loopStabilize, NULL);
	if (err != 0) {
		printf("can't create a pthread of looping stablize\n");
		abort();
	}

	if (port > DEFAULT_PORT)
		join();

/*
	if (menu) {
		printMenu();
	}
*/	
	pthread_join(tid[0],NULL);
	pthread_join(tid[1],NULL);
	
	pthread_mutex_destroy(&lock);
	
	mpz_clear(id);

	return 0;
}


/**
 * Initialize the server socket
 * @param port2 Server port number
 */
void initServerSocket(uint16_t port) {
	struct sockaddr_in servAddr;
	// struct sockaddr_in servAddrTCP;

	connfdUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// connfdTCP = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	/* Immediate bind port without waiting until OS releases*/
	/*
	int iSetOption = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption,
        sizeof(iSetOption));
	*/
	
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(port);

	// memset(&servAddrTCP, 0, sizeof(servAddrTCP));
	// servAddrTCP.sin_family = AF_INET;
	// servAddrTCP.sin_addr.s_addr = htonl(INADDR_ANY);
	// servAddrTCP.sin_port = htons(port + 2000);

	bind(connfdUDP, (struct sockaddr*)&servAddr, sizeof(servAddr));
	// bind(connfdTCP, (struct sockaddr*)&servAddrTCP, sizeof(servAddrTCP));
	fprintf(stderr, "init serversocket done\n");
	fflush(stderr);
}

void listenServerTCPSocket() {	
	fprintf(stderr, "server: TCP listening... at\n");

	listen(connfdTCP, 5);
	
	int sockfd;
	char recvBuf[512];
	mpz_t sId; mpz_init(sId);
	mpz_t targetId; mpz_init(targetId);
	char ipAddr[IPADDR_SIZE];
	uint16_t port = 0;
	// int dataSize = 0;
	// int size = 0;
	// char* buf = NULL;

	while (true) {
		sockfd = accept(connfdTCP, (struct sockaddr*)NULL, NULL);

		//receive messages
		if (read(sockfd, recvBuf, 1024) < 0) {
			close(sockfd);
		}

		memset(ipAddr, 0, IPADDR_SIZE);
		int pktType = parse(recvBuf, targetId, sId, ipAddr, &port, NULL, NULL);

		switch (pktType) {
		case -1:
			fprintf(stderr, "server: error occured\n");
			close(sockfd);
			break;
#if 0
		case REQ_GET_KEY: // keys removed and be transfered
			mpz_t keySize; mpz_init(keySize);
			mpz_t keys[1024];
			for (i = 0; i < 1024; ++i) {
				mpz_init(keys[i]);
			}

			char* data[DATA_SIZE];
			getKeys(sId, keys, keySize, data);
			size = sizeof(char)*4 + sizeof(char)*keySize;
			buf = malloc(size);

			//sending keySize
			createResPkt(buf, NULL, NULL, NULL, 0, keySize, RES_GET_KEY_SIZE);
			write(sockf, buf, size);
			free(buf);
			//sending key + data
			buf = malloc(size);
			createResKeyPkt(buf, keySize, data, RES_GET_KEY_SIZE);
			write(sockfd, buf, size);
			close(sockfd);
			free(buf);
			for (i = 0; i < 1024; ++i) {
				free(data[i]);
			}
			break;
#endif

		default:
			fprintf(stderr, "server: default\n");
			close(sockfd);
			break;
		}
	}

	mpz_clear(sId); mpz_clear(targetId);
}

/**
 * Listening to the server socket
 * @return -1 if read error occures
 */
void listenServerUDPSocket() {
	fprintf(stderr, "server: UDP listening... \n");

	char recvBuf[128];
	char* sendBuf;
	
	struct sockaddr_in cliAddr;
	socklen_t len = 0;					// size of the client addr struct

	mpz_t targetId; mpz_init(targetId);
	char ipAddr[IPADDR_SIZE];
	uint16_t port = 0;
	mpz_t predId; mpz_init(predId);
	uint16_t predPort = 0;

	mpz_t sId; mpz_init(sId);
	uint16_t sPort = 0;
	int size = 0;

	while (true) {
		len = sizeof(cliAddr);
		memset(&cliAddr, 0, len);
		memset(ipAddr, 0, IPADDR_SIZE);
		memset(recvBuf, 0, 128);
	
		//receive messages
		int n = recvfrom(connfdUDP, recvBuf, 128, 0, (struct sockaddr*)&cliAddr, &len);
		if (n < 0) {
			fprintf(stderr, "received packet < 0 \n");
			continue;
		}
		int pktType = parse(recvBuf, targetId, sId, ipAddr, &port, NULL, NULL);

		switch (pktType) {
		case ERROR: //error occured
			fprintf(stderr, "server: error occured\n");
			break;
		
		case REQ_FIND_CLOSEST_FINGER: // Find closest finger preceding
			size = sizeof(char) * 28;
			sendBuf = malloc(size);
			int ret = closestPrecedingFinger(targetId, sId, ipAddr, &port);
			if (ret == true) { // found
				createResPkt(sendBuf, NULL, sId, ipAddr, port, 0, RES_FIND_CLOSEST_FINGER_FOUND);
			}  else {
				createResPkt(sendBuf, NULL, sId, ipAddr, port, 0, RES_FIND_CLOSEST_FINGER_NOTFOUND);
			}
			sendto(connfdUDP, sendBuf, size, 0, (struct sockaddr*)&cliAddr, len);
			free(sendBuf);
			break;
		
		case REQ_GET_PRED: // Req Predecessor
			// fprintf(stderr, "Recieved: REQ_GET_PRED\n");			
			size = sizeof(char) * 28;
			sendBuf = malloc(size);
			getPredecesor(predId, ipAddr, &predPort);
			createResPkt(sendBuf, NULL, predId, ipAddr, predPort, 0, RES_GET_PRED);
			sendto(connfdUDP, sendBuf, size, 0, (struct sockaddr*)&cliAddr, len);
			free(sendBuf);
			break;
		
		case REQ_GET_SUCC:  // reply the successor
			// fprintf(stderr, "Recieved: REQ_GET_SUCC\n");
			size = sizeof(char) * 28;
			sendBuf = malloc(size);	
			getSuccessor(sId, ipAddr, &sPort);
			createResPkt(sendBuf, NULL, sId, ipAddr, sPort, 0, RES_GET_SUCC);
			sendto(connfdUDP, sendBuf, size, 0, (struct sockaddr*)&cliAddr, len);
			free(sendBuf);
			break;
		
		case REQ_MODIFY_PRED:  //modify its predecessor
			// fprintf(stderr, "Recieved: Modify \n");
			modifyPred(sId, ipAddr, port, false);
			break;
		
		case REQ_MODIFY_PRED_TIME_OUT:  //modify its predecessor when timeout
			// fprintf(stderr, "Recieved: Modify \n");
			modifyPred(sId, ipAddr, port, true);
			break;

		case REQ_CHECK_ALIVE:
			// fprintf(stderr, "Recieved: REQ_CHECK_ALIVE\n");
			size = sizeof(char) * 2;
			sendBuf = malloc(size);
			createResPkt(sendBuf, NULL, NULL, NULL, 0, 0, RES_CHECK_ALIVE);
			sendto(connfdUDP, sendBuf, size, 0, (struct sockaddr*)&cliAddr, len);
			free(sendBuf);
			break;

		case REQ_START_SIM:
			// printf("START Simulation\n");
			// fflush(stdout);
			// pthread_create(&(tid[2]), NULL, (void*)&sim_failure, NULL);
			pthread_create(&(tid[2]), NULL, (void*)&sim_pathLength, NULL);
			break;

		case REQ_ABORT:
			printf("Aborted\n"); fflush(stdout);
			abort();
			break;

		default:
			fprintf(stderr, "server: default\n");
			break;
		}
	}

	mpz_clear(targetId); mpz_clear(predId); mpz_clear(sId);
}

/**
 * Periodically stablizing
 * @return [description]
 */
void loopStabilize() {

	while (1) {
		usleep(500 * 1000);
		stabilize();
	}	
	pthread_exit(0); //exit
}

/**
 * Version: 1 byte - 0xC0
 * Request: 1 byte - 0x0
 * Port: 	2 byte
 * IPv4: 	4 byte
 * Node ID:	4 byte
 * Target ID:4 byte
 * @param  buf [description]
 * @return     [description]
 */
void createReqPkt(char* buf, mpz_t targetId, mpz_t sId, char* ipAddr, 
					uint16_t port, int pktType) {

	int i = 0; int j = 0;

	switch (pktType) {
	case REQ_FIND_CLOSEST_FINGER:
		buf[0] = 0xC0;
		buf[1] = pktType & 0xFF;

		buf[2] = 0x00;
		buf[3] = 0x00;

		if (targetId != NULL) {
			unsigned char targetIdStr[SHA_DIGEST_LENGTH];
			mpzToByteArray(targetId, targetIdStr);
			for (i = SHA_DIGEST_LENGTH-1; i >= 0; --i) {
				buf[j+4] = targetIdStr[i] & 0xFF;
				j++;
			}
		}
		break;

	case REQ_GET_PRED:
	case REQ_GET_SUCC:
		buf[0] = 0xC0;
		buf[1] = pktType & 0xFF;
		break;

	case REQ_MODIFY_PRED:
	case REQ_MODIFY_PRED_TIME_OUT:
		buf[0] = 0xC0;
		buf[1] = pktType & 0xFF;

		buf[2] = port >> 8; 	// Port_high
		buf[3] = port & 0xFF;	// Port_low

		struct sockaddr_in tmp_addr;
		tmp_addr.sin_addr.s_addr = inet_addr(ipAddr);
		buf[4] = (tmp_addr.sin_addr.s_addr >> 8*3) & 0xFF;	//IP Adr high
		buf[5] = (tmp_addr.sin_addr.s_addr >> 8*2) & 0xFF;
		buf[6] = (tmp_addr.sin_addr.s_addr >> 8*1) & 0xFF;
		buf[7] =  tmp_addr.sin_addr.s_addr & 0xFF;			//IP Adr low
		
		if (sId != NULL) {
			unsigned char sIdStr[SHA_DIGEST_LENGTH];
			mpzToByteArray(sId, sIdStr);
			// for (i = 0; i < SHA_DIGEST_LENGTH; ++i) {
			// 	buf[i+8] = sIdStr[i];
			// }
			for (i = SHA_DIGEST_LENGTH-1; i >= 0; --i) {
				buf[j+8] = sIdStr[i] & 0xFF;
				j++;
			}
		}

		break;

	case REQ_CHECK_ALIVE:
		buf[0] = 0xC0;
		buf[1] = pktType & 0xFF;
		break;

	case REQ_GET_KEY:
		buf[0] = 0xC0;
		buf[1] = pktType & 0xFF;

		if (sId != NULL) {
			unsigned char sIdStr[SHA_DIGEST_LENGTH];
			mpzToByteArray(sId, sIdStr);
			// for (i = 0; i < SHA_DIGEST_LENGTH; ++i) {
			// 	buf[i+2] = sIdStr[i];
			// }
			for (i = SHA_DIGEST_LENGTH-1; i >= 0; --i) {
				buf[j+2] = sIdStr[i] & 0xFF;
				j++;
			}
		}
		break;

	default:
		break;

	}
}

/**
 * Version: 	1 byte - 0xC0
 * Response: 	1 byte - 0x02: no more request 0x01: more request
 * Port:		2 byte
 * IP:			4 byte
 * S Node ID:	4 byte
 * Target ID:	4 byte
 * @param  buf [description]
 * @return     [description]
 */
void createResPkt(char* buf, mpz_t targetId, mpz_t sId, char* ipAddr, 
					uint16_t port, int keySize, int pktType) {
	int i = 0; int j = 0;
	switch (pktType) {
	case RES_FIND_CLOSEST_FINGER_FOUND:
	case RES_FIND_CLOSEST_FINGER_NOTFOUND:
	case RES_GET_PRED:
	case RES_GET_SUCC:
		buf[0] = 0xC0;
		buf[1] = pktType & 0xFF;

		buf[2] = port >> 8; 	// Port_high
		buf[3] = port & 0xFF;	// Port_low

		struct sockaddr_in tmp_addr;
		tmp_addr.sin_addr.s_addr = inet_addr(ipAddr);
		buf[4] = (tmp_addr.sin_addr.s_addr >> 8*3) & 0xFF;	//IP Adr high
		buf[5] = (tmp_addr.sin_addr.s_addr >> 8*2) & 0xFF;
		buf[6] = (tmp_addr.sin_addr.s_addr >> 8*1) & 0xFF;
		buf[7] =  tmp_addr.sin_addr.s_addr & 0xFF;			//IP Adr low
		
		// Succesor NODE ID
		if (sId != NULL) {
			unsigned char sIdStr[SHA_DIGEST_LENGTH];
			mpzToByteArray(sId, sIdStr);
			// for (i = 0; i < SHA_DIGEST_LENGTH; ++i) {
			// 	buf[i+8] = sIdStr[i];
			// }
			for (i = SHA_DIGEST_LENGTH-1; i >= 0; --i) {
				buf[j+8] = sIdStr[i] & 0xFF;
				j++;
			}
		}

		break;
	
	case RES_CHECK_ALIVE:
		buf[0] = 0xC0;
		buf[1] = pktType & 0xFF;
		break;

/**	
	case RES_GET_KEY_SIZE:
		buf[0] = 0xC0;
		buf[1] = pktType & 0xFF;

		break;
**/
	default:
		break;
	}
}

/**
 * Create a key-transfer response packet
 * @param buf  [description]
 * @param size [description]
 * @param keys [description]
 * @param keySize  [description]
 * @param type [description]
 */
void createResKeyPkt(char* buf, mpz_t key, char* data, int dataSize, int pktType) {
	if (dataSize < 0) {
		fprintf(stderr, "Size should be bigger than 0\n");
	}

	buf[0] = 0xC0;
	buf[1] = pktType & 0xFF;

	buf[2] = dataSize >> 8;     // NUM HIGH
	buf[3] = dataSize & 0xFF;   // NUM LOW

	int i = 0;
	for (i = 0; i < dataSize; ++i) {
		buf[i+4] = data[i]; 
	}
}

/**
 * Parse a received packet
 * @param  buf      [description]
 * @param  targetId [description]
 * @param  sId      [description]
 * @param  ipAddr   [description]
 * @param  port     [description]
 * @return          packet type
 */
int parse(char buf[], mpz_t targetId,  mpz_t sId, char* ipAddr, uint16_t* port, 
			unsigned char* data, int* dataSize) {

	if ((buf[0] & 0xFF) != 0xC0) {
		fprintf(stderr, "Received packet does NOT have 0xC0\n");
		return ERROR;
	}

	int pktType = buf[1] & 0xFF;
	int i = 0;
	
	switch (pktType) {
	case REQ_FIND_CLOSEST_FINGER:
		if (targetId != NULL) {
			unsigned char targetIdStr[SHA_DIGEST_LENGTH];
			for (i = 0; i < SHA_DIGEST_LENGTH; ++i) {
				targetIdStr[i] = buf[i+4];
			}
			ByteArrayToMpz(targetId, targetIdStr);
		}
		break;
	
	case RES_FIND_CLOSEST_FINGER_FOUND:
	case RES_FIND_CLOSEST_FINGER_NOTFOUND:
	case RES_GET_PRED:
	case RES_GET_SUCC:
	case REQ_MODIFY_PRED:
	case REQ_MODIFY_PRED_TIME_OUT:
		*port = ((buf[2] & 0xFF) << 8) | (buf[3] & 0xFF);
		uint32_t ip = (buf[4]&0xFF) << 8*3 | (buf[5]&0xFF) << 8*2 
					| (buf[6]&0xFF) << 8 | (buf[7]&0xFF);
		inet_ntop(AF_INET, &(ip), ipAddr, INET_ADDRSTRLEN);
		
		if (sId != NULL) {
			unsigned char sIdStr[SHA_DIGEST_LENGTH];
			for (i = 0; i < SHA_DIGEST_LENGTH; ++i) {
				sIdStr[i] = buf[i+8];
			}
			ByteArrayToMpz(sId, sIdStr);
		}
		break;

	case REQ_GET_KEY:
		if (sId != NULL) {
			unsigned char sIdStr[SHA_DIGEST_LENGTH];
			for (i = 0; i < SHA_DIGEST_LENGTH; ++i) {
				sIdStr[i] = buf[i+2];
			}
			ByteArrayToMpz(sId, sIdStr);
		}
		break;
		
#if 0
	case RES_GET_KEY:
		*dataSize = ((buf[2] & 0xFF) << 8) | (buf[3] & 0xFF);
		for (i = 0; i < *dataSize; ++i) {
			data[i] = buf[i+4];
		}
		break;
#endif

	default:
		break;
	}

	return pktType;
}

/**
 * Send a Request closest finger preceding Packet
 * @param  targetId [description]
 * @param  sId      [description]
 * @param  ipAddr   [description]
 * @param  port     [description]
 * @return          [description]
 */
void sendReqClosestFingerPkt(int sockfd, mpz_t targetId, char* sIpAddr, uint16_t sPort) {
	struct sockaddr_in cliAddr;
	memset(&cliAddr, 0, sizeof(cliAddr));
	cliAddr.sin_family = AF_INET;
	cliAddr.sin_addr.s_addr = inet_addr(sIpAddr);
	cliAddr.sin_port = htons(sPort);

	int size = sizeof(char) * 24;
	char* buf = malloc(size);
	memset(buf, 0, size);

	createReqPkt(buf, targetId, NULL, NULL, 0, REQ_FIND_CLOSEST_FINGER);
	sendto(sockfd, buf, size, 0, (struct sockaddr*)&cliAddr, sizeof(cliAddr));

	free(buf);
}

/**
 * Send an ASK packet to the successor for its predecessor
 * @param  sId     Successor ID to be asked
 * @param  sIpAddr Successor IP Address to be asked
 * @param  sPort   Successor Port to be asked
 * @return         -1 if error, 0 if successful
 */
void sendReqSuccForPredPkt(int sockfd, char* sIpAddr, uint16_t sPort) {
	struct sockaddr_in cliAddr;
	memset(&cliAddr, 0, sizeof(cliAddr));
	cliAddr.sin_family = AF_INET;
	cliAddr.sin_addr.s_addr = inet_addr(sIpAddr);
	cliAddr.sin_port = htons(sPort);

	int size = sizeof(char) * 2;
	char* buf = malloc(size);
	memset(buf, 0, size);

	createReqPkt(buf, NULL, NULL, NULL, 0, REQ_GET_PRED);
	sendto(sockfd, buf, size, 0, (struct sockaddr*)&cliAddr, sizeof(cliAddr));

	free(buf);
}

/**
 * Send an ASK pakcet to the successor for its successor
 * @param  sId     Successor ID to be asked
 * @param  sIpAddr Successor IP Address to be asked
 * @param  sPort   Successor Port to be asked
 * @return         -1 if error, 0 if successful
 */
void sendReqSuccForSuccPkt(int sockfd, char* sIpAddr, uint16_t sPort) {
	struct sockaddr_in cliAddr;
	memset(&cliAddr, 0, sizeof(cliAddr));
	cliAddr.sin_family = AF_INET;
	cliAddr.sin_addr.s_addr = inet_addr(sIpAddr);
	cliAddr.sin_port = htons(sPort);

	int size = sizeof(char) * 2;
	char* buf = malloc(size);
	memset(buf, 0, size);

	createReqPkt(buf, NULL, NULL, NULL, 0, REQ_GET_SUCC);
	sendto(sockfd, buf, size, 0, (struct sockaddr*)&cliAddr, sizeof(cliAddr));

	free(buf);
}

/**
 * Send a Notify Packet to the Seccessor ID
 * @param  sId     Successor ID to 
 * @param  sIpAddr [description]
 * @param  sPort   [description]
 * @param  id      [description]
 * @param  ipAddr  [description]
 * @param  port    [description]
 * @return         -1 if error, 0 if success
 */
void sendNotifyPkt(int sockfd, char* sIpAddr, uint16_t sPort,
					mpz_t id, char* ipAddr, uint16_t port, bool timeout) {

	struct sockaddr_in cliAddr;
	memset(&cliAddr, 0, sizeof(cliAddr));
	cliAddr.sin_family = AF_INET;
	cliAddr.sin_addr.s_addr = inet_addr(sIpAddr);
	cliAddr.sin_port = htons(sPort);

	int size = sizeof(char) * 28;
	char* buf = malloc(size);
	memset(buf, 0, size);

	if (timeout)
		createReqPkt(buf, 0, id, ipAddr, port, REQ_MODIFY_PRED_TIME_OUT);
	else
		createReqPkt(buf, 0, id, ipAddr, port, REQ_MODIFY_PRED);

	sendto(sockfd, buf, size, 0, (struct sockaddr*)&cliAddr, sizeof(cliAddr));

	free(buf);
}

void sendReqSuccForKeyPkt(int sockfd, mpz_t id, char* sIpAddr, uint16_t sPort) {
	
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(sPort);
	if(inet_pton(AF_INET, sIpAddr, &servAddr.sin_addr) < 0) {
		fprintf(stderr, "converting %s from a string error occured\n", sIpAddr);
		return;
	}
	if(connect(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr))<0){
		fprintf(stderr, "%d %s Connection failed %s: %lu, %s\n", __LINE__, __FILE__, sIpAddr, (unsigned long) sPort, strerror(errno));
		return;
	}

	int size = sizeof(char) * (2 + SHA_DIGEST_LENGTH);
	char* buf = malloc(size);
	createReqPkt(buf, NULL, id, NULL, 0, REQ_GET_KEY);
	write(sockfd, buf, size);
	free(buf);
}

void sendReqAlivePkt(int sockfd, char* ipAddr, uint16_t port) {
	struct sockaddr_in cliAddr;
	memset(&cliAddr, 0, sizeof(cliAddr));
	cliAddr.sin_family = AF_INET;
	cliAddr.sin_addr.s_addr = inet_addr(ipAddr);
	cliAddr.sin_port = htons(port);

	int size = sizeof(char) * 2;
	char* buf = malloc(size);
	memset(buf, 0, size);

	createReqPkt(buf, 0, 0, NULL, 0, REQ_CHECK_ALIVE);
	sendto(sockfd, buf, size, 0, (struct sockaddr*)&cliAddr, sizeof(cliAddr));

	free(buf);
}

/**
 * Receive a response pakcet 
 * @param  targetId [description]
 * @param  sId      [description]
 * @param  sIpAddr  [description]
 * @param  sPort    [description]
 * @return          [description]
 */
int recvResPkt(int sockfd, mpz_t sId, char* sIpAddr, uint16_t* sPort) {
	char buf[128];
	recvfrom(sockfd, buf, 128, 0, NULL, NULL);
	int pktType = parse(buf, NULL, sId, sIpAddr, sPort, NULL, NULL);
	return pktType;
}


/**
 * Receive a key-transfer packet and parse it
 * @param  keys 	Keys
 * @param  keySize  The number of keys
 * @return      	-1 if error, 6 or 7 if successful
 */
int recvResKeyPkt(int sockfd, unsigned char* data, int* dataSize) {
	char buf[DATA_SIZE+4];
	int n = read(sockfd, buf, DATA_SIZE+4);
	if(n < 0) {
		close(sockfd);
		return ERROR;
	}
	int pktType = parse(buf, NULL, NULL, NULL, 0, data, dataSize);
	close(sockfd);
	return pktType;
}

#if 0
/**
 * Send a Key-transfer packet
 * @param  sId     [description]
 * @param  sIpAddr [description]
 * @param  sPort   [description]
 * @param  keys    [description]
 * @param  keySize [description]
 * @return         [description]
 */
void sendKeyTransPkt(uint32_t sId, char* sIpAddr, uint16_t sPort, uint32_t keys[], int keySize) {
	int sockfd = 0;
	int n = connectToServer(&sockfd, sIpAddr, sPort);
	if (n < 0) {
		fprintf(stderr, "Connection to %s, %lu failed\n", sIpAddr, (unsigned long)sPort);
		close(sockfd);
		return -1;
	}

	int size = sizeof(char)*keySize*4 + sizeof(char)*4;
	char* buf = malloc(size);
	createKeyTransPkt(buf, size, keys, keySize, 7);//7 when key-transfer for failure
	writeToSocket(sockfd, buf, size);
	
	free(buf);
	close(sockfd);
}
#endif

/**
* Connect the IP address
* @param addr To be connected
* @param port To be connected
* @return -1 if error, 0 if successful
*/
/* TODO: Close socket fd from client */
int connectToServer(int* sockfd, char* ipAddr, uint16_t port) {
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	if((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "openning socket error occured\n");
		return -1;
	}
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(port);
	if(inet_pton(AF_INET, ipAddr, &servAddr.sin_addr) < 0) {
		fprintf(stderr, "converting %s from a string error occured\n", ipAddr);
		return -1;
	}
	if(connect(*sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr))<0){
		fprintf(stderr, "%d %s Connection failed %s: %lu, %s\n", __LINE__, __FILE__, ipAddr, (unsigned long) port, strerror(errno));
		return -1;
	}
	return 0;
}


/**
 * Print menu
 */
void printMenu() {
#if 0
	char *prompt = "> ";

	while (true) {
		printf("\nMain Menu\n");
		printf("1) Retreive:\n");
		printf("2) Print all information\n");
		printf("3) Exit\n");

		printf("%s", prompt);

		int c = 0;
		scanf("%d", &c);

		switch (c) {
		case 1:
			printf("Input number: ");
			int tmp = 0;
			scanf("%d", &tmp);
			uint32_t id = (uint32_t)tmp;
			uint32_t sId = 0;
			char ipAddr[15];
			uint16_t sPort = 0;
			if (findSuccessor(id, &sId, ipAddr, &sPort) > 0){
				printf("Found: ");
				printf("%lu %s %lu", (unsigned long)sId, ipAddr, (unsigned long)sPort);
			} else {
				printf("Not Found\n");
			}
			break;
		case 2:
			printDebug();
			printFT();
			printSuccList();
			break;
		case 3:
			printf("Exit\n");
			break;
		default:
			printf("Chose another menu\n");
			break;
		}
	}
#endif
}