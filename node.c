/** @file node.c
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#include "node.h"
#include "chord.h"

/**
 * initialize node
 * @param  nodeId     [description]
 * @return            [description]
 */
int initNode(uint32_t nodeId) {
	int err = -1;
	uint16_t port = DEFAULT_PORT + nodeId;

	initChord(nodeId);

	//create a thread of server socket 
	err = pthread_create(&(tid[0]), NULL, (void*)&initServerSocket, (void*) &port);
	if (err != 0) {
		printf("can't create a pthread of a server socket\n");
		abort();
	}

	//create a thread of looping stablizing
	err = pthread_create(&(tid[1]), NULL, (void*)&loopStablize, NULL);
	if (err != 0) {
		printf("can't create a pthread of looping stablize\n");
		abort();
	}

	//NOT recommended, but pthread_cancel() is used to terminate pthreads
	// pthread_cancel(tid[0]);
	// pthread_cancel(tid[1]);
	
	pthread_join(tid[0],NULL);
	pthread_join(tid[1],NULL);
	
	return 0;
}

/**
 * Initialize the server socket
 * @param port2 [description]
 */
void initServerSocket(void* port2) {
	uint16_t* port = (uint16_t*) port2;
	listenfd = 0;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&servAdr, 0, sizeof(servAdr));
	memset(sendBufServ, 0, sizeof(sendBufServ));

	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(*port);

	bind(listenfd, (struct sockaddr*)&servAdr, sizeof(servAdr));

#ifdef DEBUG
	printf("server: listening... at %lu\n", (unsigned long) *port);
#endif

	listenServerSocket();
}

int listenServerSocket() {
	listen(listenfd, 100);

	while(1) {
		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

		//receive messages
		if (readFromSocket(connfd, recvBufServ) < 0) {
			printf("read error");
			return -1;
		}

		uint32_t targetId = 0;
		uint32_t successorId = 0;
		char ipAddr[15];
		uint16_t port = 0;
		char buf[16];

		int ret = parse(recvBufServ, &targetId, &successorId, ipAddr, &port);
		switch (ret) {
			case -1: //error occured
				printf("server: error occured\n");
				break;
			case 1: //request
				printf("server: find successor request received\n");				
				int n = 0; 
				n=closestPrecedingFinger(targetId, &successorId, ipAddr, &port);
				createResPkt(buf, targetId, successorId, ipAddr, port, n);
				writeToSocket(connfd, buf);
				closeSocket(connfd);
				break;
			case 3:
				printf("server: ask for predecesor request received\n");				
				uint32_t predId = 0;
				uint16_t predPort = 0;
				memset(ipAddr, 0, 15);  // clear ipAddr
				getPredecesor(&predId, ipAddr, &predPort);
				createResPkt(buf, 0, predId, ipAddr, predPort, n);
				writeToSocket(connfd, buf);
				closeSocket(connfd);
				break;
			case 4:  //modify its predecessor
				predId = successorId;
				modifyPred(predId, ipAddr, port);
				break;
			case 5:  // reply the successor
				printf("server: ask for successor request received\n");				
				uint32_t sId = 0;
				uint16_t sPort = 0;
				memset(ipAddr, 0, 15);  // clear ipAddr
				getSuccessor(&sId, ipAddr, &sPort);
				createResPkt(buf, 0, sId, ipAddr, sPort, n);
				writeToSocket(connfd, buf);
				closeSocket(connfd);
				break;
			case 6:  // keys removed and be transfered
				printf("server: ask for key request receieved\n");
				uint32_t keys[NUM_KEYS];
				int num = 0;
				getKeys(targetId, keys, &num);
				if (nd->ndInfo.id == 5 )
					printf("dddddddddd %d   %d\n", targetId, num);
				createKeyResPkt(buf, keys, num);
				writeToSocket(connfd, buf);
				closeSocket(connfd);
				break;
			default:
				printf("server: deafult\n");
				break;
		}
	}
}

/**
 * Convert IP address from a string of addr
 * Connect to addr and port
 * @param  addr [description]
 * @param  port [description]
 * @return      [description]
 */
/* TODO: Close socket fd from client */
int connectToServer(char* ipAddr, uint16_t port) {
	memset(&servAdrCli, 0, sizeof(servAdrCli));
	
	if((connfdCli = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("openning socket error occured\n");
	}
	servAdrCli.sin_family = AF_INET;
	// if (port == 0) 
		// port = DEFAULT_PORT;
	servAdrCli.sin_port = htons(port);

	if(inet_pton(AF_INET, ipAddr, &servAdrCli.sin_addr) <= 0) {
		printf("converting %s from a string error occured\n", ipAddr);
		return -1;
	}

	if(connect(connfdCli, (struct sockaddr*)&servAdrCli, sizeof(servAdrCli))<0){
		printf("Connection failed %s: %lu, %s\n", ipAddr, (unsigned long) port,
			strerror(errno));
		return -1;
	}

	return 0;
}

int checkConnection(char* ipAddr, uint16_t port) {
	int n = connectToServer(ipAddr, port);
	closeSocket(connfdCli);

	return n;
}

int readFromSocket(int fd, char* buf) {
	int n = read(fd, buf, 16);
	return n;
}

int writeToSocket(int fd, char* buf) {
	write(fd, buf, 16);
	return 0;
}

/**
 * Periodically stablizing
 * @return [description]
 */
void loopStablize() {
	
	while (1) {
		sleep(3);
		stabilize();
		fixFingers();
	}	
	pthread_exit(0); //exit
}

int closeSocket(int socketfd) {
	close(socketfd);
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
void createReqPkt(char* buf, uint32_t targetId, uint32_t successorId, 
					char* ipAddr, uint16_t port, int res) {

	memset(buf, 0, 16);

	buf[0] = 0xC0; 			// Version
	buf[1] = res & 0xFF;	// 0: FindSuccessor, 3:Ask Successor for predecesor
	
	buf[2] = port >> 8; 	// Port_high
	buf[3] = port & 0xFF;	// Port_low

	struct sockaddr_in tmp_addr;
	tmp_addr.sin_addr.s_addr = inet_addr(ipAddr);
	buf[4] = (tmp_addr.sin_addr.s_addr >> 8*3) & 0xFF;	//IP Adr high
	buf[5] = (tmp_addr.sin_addr.s_addr >> 8*2) & 0xFF;
	buf[6] = (tmp_addr.sin_addr.s_addr >> 8*1) & 0xFF;
	buf[7] = tmp_addr.sin_addr.s_addr & 0xFF;			//IP Adr low

	//NODE ID
	buf[8] = (successorId >> 8*3) & 0xFF;	//NODE ID HIGH
	buf[9] = (successorId >> 8*2) & 0xFF;
	buf[10] = (successorId >> 8*1) & 0xFF;
	buf[11] = successorId & 0xFF;			//NODE ID LOW

	// Target Node ID
	buf[12] = (targetId >> 8*3) & 0xFF;	//NODE ID HIGH
	buf[13] = (targetId >> 8*2) & 0xFF;
	buf[14] = (targetId >> 8*1) & 0xFF;
	buf[15] = targetId & 0xFF;			//NODE ID LOW
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
void createResPkt(char* buf, uint32_t targetId, uint32_t successorId, 
					char* ipAddr, uint16_t port, int res) {

	memset(buf, 0, 16);

	buf[0] = 0xC0;
	buf[1] = res & 0xFF;
	
	buf[2] = port >> 8; 	// Port_high
	buf[3] = port & 0xFF;	// Port_low

	struct sockaddr_in tmp_addr;
	tmp_addr.sin_addr.s_addr = inet_addr(ipAddr);
	buf[4] = (tmp_addr.sin_addr.s_addr >> 8*3) & 0xFF;	//IP Adr high
	buf[5] = (tmp_addr.sin_addr.s_addr >> 8*2) & 0xFF;
	buf[6] = (tmp_addr.sin_addr.s_addr >> 8*1) & 0xFF;
	buf[7] = tmp_addr.sin_addr.s_addr & 0xFF;			//IP Adr low

	//Succesor NODE ID
	buf[8] = (successorId >> 8*3) & 0xFF;	//NODE ID HIGH
	buf[9] = (successorId >> 8*2) & 0xFF;
	buf[10] = (successorId >> 8*1) & 0xFF;
	buf[11] = successorId & 0xFF;			//NODE ID LOW

	// Target Node ID
	buf[12] = (targetId >> 8*3) & 0xFF;	//NODE ID HIGH
	buf[13] = (targetId >> 8*2) & 0xFF;
	buf[14] = (targetId >> 8*1) & 0xFF;
	buf[15] = targetId & 0xFF;			//NODE ID LOW
}

/**
 * 
 * @param buf  [description]
 * @param keys [description]
 * @param num  [description]
 */
/* FIXME: Num is 4bytes, but in my protocol, only 2bytes allowed
only three keys are allowed
*/
void createKeyResPkt(char* buf, uint32_t keys[], int num) {
	memset(buf, 0 , 16);

	buf[0] = 0xC0;
	buf[1] = 6 & 0xFF;

	buf[2] = num >> 8;     // NUM HIGH
	buf[3] = num & 0xFF;   // NUM LOW

	int i = 0; int j = 4;
	for (i = 0; i < num; ++i) {
		if (num > 3)
			break;
		buf[j++] = (keys[i] >> 8*3) & 0xFF; 
		buf[j++] = (keys[i] >> 8*2) & 0xFF;
		buf[j++] = (keys[i] >> 8*1) & 0xFF;
		buf[j++] =  keys[i] & 0xFF;
	}
}
/**
 * Parse the received packet
 * @param  buf [description]
 * @return     [description]
 */
int parse(char* buf, uint32_t* targetId, uint32_t* successorId, 
			char* ipAddr, uint16_t* port) {
	
	if (buf[0] & 0xFF != 0xC0) {
		printf("Received packet not have 0xC0\n");
		return -1;
	}

	// port, assemble with the reverse
	*port = (buf[2] << 8) | buf[3];

	//IP
	uint32_t ip = buf[4] << 8*3 | buf[5] << 8*2 | buf[6] << 8 | buf[7];
	inet_ntop(AF_INET, &(ip), ipAddr, INET_ADDRSTRLEN);
	*successorId = (buf[8] << 8*3) | (buf[9] << 8*2) | (buf[10] << 8) | buf[11];
	*targetId = (buf[12] << 8*3) | (buf[13] << 8*2) | (buf[14] << 8) | buf[15];

	return buf[1];
}
int parseForKeyResPkt(char* buf, uint32_t keys[], int* num) {
	if (buf[0] & 0xFF != 0xC0) {
		printf("Received packet not have 0xC0\n");
		return -1;
	}

	*num = (buf[2] << 8) | buf[3];

	//keys
	int i = 0; int j = 4;
	for (i = 0; i < *num; ++i) {
		keys[i] = buf[j++] << 8*3 | buf[j++] << 8*2 | buf[j++] << 8 | buf[j++];
	}
	return buf[1];

}
int sendReqPkt(uint32_t targetId, uint32_t successorId, 
					char* ipAddr, uint16_t port) {
	int n = connectToServer(ipAddr, port);
	if (n == -1) { //conection failed
		return n;
	}
	char buf[16];
	createReqPkt(buf, targetId, successorId, ipAddr, port, 1);
	writeToSocket(connfdCli, buf);
}

int sendAskSuccForPredPkt(uint32_t sId, char* sIpAddr, uint16_t sPort) {
	int n = connectToServer(sIpAddr, sPort);
	if (n < 0) {
		printf("Connection to %s, %lu failed\n", sIpAddr, (unsigned long)sPort);
		return -1;
	}
	char buf[16];
	createReqPkt(buf, 0, sId, sIpAddr, sPort, 3);
	writeToSocket(connfdCli, buf);
}

int sendAskSuccForSuccPkt(uint32_t sId, char* sIpAddr, uint16_t sPort) {
	int n = connectToServer(sIpAddr, sPort);
	if (n < 0) {
		printf("Connection to %s, %lu failed\n", sIpAddr, (unsigned long)sPort);
		return -1;
	}
	char buf[16];
	createReqPkt(buf, 0, sId, sIpAddr, sPort, 5);
	writeToSocket(connfdCli, buf);
}

int sendAskSuccForKeyPkt(uint32_t id, uint32_t sId, char* sIpAddr, uint16_t sPort) {
	int n = connectToServer(sIpAddr, sPort);
	if (n < 0) {
		printf("Connection to %s, %lu failed\n", sIpAddr, (unsigned long)sPort);
		return -1;
	}
	char buf[16];
	createReqPkt(buf, id, sId, sIpAddr, sPort, 6);
	writeToSocket(connfdCli, buf);
}

int recvResPkt(uint32_t targetId, uint32_t* successorId, 
					char* ipAddr, uint16_t* port) {
	int n = readFromSocket(connfdCli, recvBufCli);
	if(n < 0) {
		printf("read error");
		return -1;
		// abort();
	}
	int ret = parse(recvBufCli, &targetId, successorId, ipAddr, port);
	closeSocket(connfdCli);
	return ret;
}

int recvKeyResPkt(uint32_t keys[], int* num) {
	int n = readFromSocket(connfdCli, recvBufCli);
	if(n < 0) {
		printf("read error");
		return -1;
	}
	int ret = parseForKeyResPkt(recvBufCli, keys, num);
printf("NNNNNNNNNNNNNN: %d\n", *num);
	closeSocket(connfdCli);
	return ret;
}

int sendNotifyPkt(uint32_t sId, char* sIpAddr, uint16_t sPort,
					uint32_t id, char* ipAddr, uint16_t port) {
	int n = connectToServer(sIpAddr, sPort);
	if (n < 0) {
		printf("Connection to %s, %lu failed\n", sIpAddr, (unsigned long)sPort);
		return -1;
		// abort();
	}
	char buf[16];
	createReqPkt(buf, sId, id, ipAddr, port, 4);
	writeToSocket(connfdCli, buf);
}