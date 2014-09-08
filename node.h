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

#define SHA_DIGEST_LENGTH 20
#define DATA_SIZE 1024
#define FT_SIZE 160
#define IPADDR_SIZE 15
#define NUM_KEYS 20
#define SLIST_SIZE 10

typedef int bool;
#define true 1
#define false 0

struct _Node;
typedef struct _Node Node;

struct NodeInfo{
	unsigned char id[SHA_DIGEST_LENGTH];
	char ipAddr[IPADDR_SIZE];
	uint16_t port;
};

struct FingerTable{
	unsigned char start[SHA_DIGEST_LENGTH]; 
	struct NodeInfo sInfo;
};
struct Successor {
	struct NodeInfo info;
	struct NodeInfo sInfo;  				/// successor Info
};

struct KeyData {
	unsigned char key[SHA_DIGEST_LENGTH];
	unsigned char data[DATA_SIZE];			/// 1 Kbyte
};

struct _Node{
	struct NodeInfo ndInfo;
	struct KeyData keyData[NUM_KEYS];
	int keySize; 							/// the number of keys
	struct NodeInfo predInfo;
	struct FingerTable ft[FT_SIZE];
	int ftSize;
	struct Successor sList[SLIST_SIZE]; 	 /// successor list
};

int connfdUDP;
int connfdTCP;

FILE *fp;

pthread_t tid[3];				/// pthread (server, client, stablizing)
pthread_mutex_t lock;			/// pthrea lock

int initNode(char* fileName, uint16_t port, int fTime, bool menu);
void initServerSocket(uint16_t port);
void listenServerTCPSocket();
void listenServerUDPSocket();
void loopStablize();

void createReqPkt(char* buf, unsigned char* targetId, unsigned char* sId, 
					char* ipAddr, uint16_t port, int pktType);
void createResPkt(char* buf, unsigned char* targetId, unsigned char* sId, 
					char* ipAddr, uint16_t port, int pktType);
void createKeyTransPkt(char* buf, int size, uint32_t keys[], int keySize, int type);

int parse(char buf[], unsigned char* targetId,  unsigned char* sId, char* ipAddr, 
			uint16_t* port, uint32_t keys[], int* keySize);
void sendReqClosestFingerPkt(int sockfd, unsigned char* targetId, char* sIpAddr, uint16_t sPort);
void sendReqSuccForPredPkt(int sockfd, unsigned char* sId, char* sIpAddr, uint16_t sPort);
void sendReqSuccForSuccPkt(int sockfd, unsigned char* sId, char* sIpAddr, uint16_t sPort);
void sendNotifyPkt(int sockfd, unsigned char* sId, char* sIpAddr, uint16_t sPort,
					unsigned char* id, char* ipAddr, uint16_t port);
void sendReqSuccForKeyPkt(int sockfd, unsigned char* id, unsigned char* sId, char* sIpAddr, uint16_t sPort);
void sendReqAlivePkt(int sockfd, char* ipAddr, uint16_t port);
int recvResPkt(int sockfd, unsigned char* sId, char* sIpAddr, uint16_t* sPort);

int connectToServer(int* sockfd, char* ipAddr, uint16_t port);
int readFromSocket(int sockfd, char* buf, int size);
int writeToSocket(int sockfd, char* buf, int size);
void printMenu();
#endif