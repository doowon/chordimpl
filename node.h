/** @file node.h
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#ifndef NODE_H
#define NODE_H

#include "packetType.h"
#include "util.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <gmp.h>

#define SHA_DIGEST_LENGTH 20
#define DATA_SIZE 1024
#define FT_SIZE 160
#define IPADDR_SIZE 15
#define SLIST_SIZE 64
#define KEY_SIZE 1024

typedef int bool;
#define true 1
#define false 0

struct _Node;
typedef struct _Node Node;

struct NodeInfo{
	mpz_t id;
	char ipAddr[IPADDR_SIZE];
	uint16_t port;
};

struct FingerTable{
	mpz_t start;
	struct NodeInfo sInfo;
};
struct Successor {
	struct NodeInfo info;
	struct NodeInfo sInfo;  				/// successor Info
};

struct KeyData {
	mpz_t key;
	unsigned char data[DATA_SIZE];			/// 1 Kbyte
	int dataSize;
};

struct _Node{
	struct NodeInfo ndInfo;
	struct KeyData keyData[KEY_SIZE];
	int keySize; 							/// the number of keys
	struct NodeInfo predInfo;
	struct FingerTable ft[FT_SIZE];
	int ftSize;
	struct Successor sList[SLIST_SIZE]; 	 /// successor list
};

int connfdUDP;
int connfdTCP;

pthread_t tid[3];				/// pthread (server, client, stablizing)
// pthread_mutex_t lock;			/// pthrea lock

/* For simulation */
mpz_t simKeys[512];
int simSize;
void sim_pathLength();
/* simulation end */

int initNode(char* idFile, char* keysFile, char* lkupFile, uint16_t port, int fTime, bool menu);
void initServerSocket(uint16_t port);
void listenServerTCPSocket();
void listenServerUDPSocket();
void loopStabilize();

void createReqPkt(char* buf, mpz_t targetId, mpz_t sId, char* ipAddr, uint16_t port, int pktType);
void createResPkt(char* buf, mpz_t targetId, mpz_t sId, char* ipAddr, uint16_t port, int keySize, int pktType);
void createResKeyPkt(char* buf, mpz_t key, char* data, int dataSize, int pktType);

int parse(char buf[], mpz_t targetId,  mpz_t sId, char* ipAddr, uint16_t* port, 
			unsigned char* data, int* dataSize);
void sendReqClosestFingerPkt(int sockfd, mpz_t targetId, char* sIpAddr, uint16_t sPort);
void sendReqSuccForPredPkt(int sockfd, char* sIpAddr, uint16_t sPort);
void sendReqSuccForSuccPkt(int sockfd, char* sIpAddr, uint16_t sPort);
void sendNotifyPkt(int sockfd, char* sIpAddr, uint16_t sPort,
					mpz_t id, char* ipAddr, uint16_t port);
void sendReqSuccForDataPkt(int sockfd, char* sIpAddr, uint16_t sPort);
void sendReqAlivePkt(int sockfd, char* ipAddr, uint16_t port);
int recvResDataPkt(int sockfd, unsigned char* data, int* dataSize);
int recvResPkt(int sockfd, mpz_t sId, char* sIpAddr, uint16_t* sPort);

int connectToServer(int* sockfd, char* ipAddr, uint16_t port);
void printMenu();

#endif