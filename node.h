/** @file node.h
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#ifndef NODE_H
#define NODE_H

#include "packetType.h"
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

#define FT_SIZE 10
#define NUM_SLIST (int)pow(2, FT_SIZE)
#define IPADDR_SIZE 15
#define NUM_KEYS 1024
#define SLIST_SIZE 1024

#define BUF_SIZE 16

typedef int bool;
#define true 1
#define false 0

struct _Node;
typedef struct _Node Node;

struct NodeInfo{
	uint32_t id;
	char ipAddr[15];
	uint16_t port;
};

struct FingerTable{
	int start; ///(n + 2^(k-1))
	struct NodeInfo sInfo;
};
struct Successor {
	struct NodeInfo info;
	struct NodeInfo sInfo;  /// successor Info
};
struct _Node{
	struct NodeInfo ndInfo;
	uint32_t key[NUM_KEYS]; 			/// Array of keys
	int keySize; 				/// the number of keys
	struct NodeInfo predInfo;
	struct FingerTable ft[FT_SIZE];
	int ftSize;
	struct Successor sList[SLIST_SIZE];  /// successor list
};

int connfdUDP;
int connfdTCP;

pthread_t tid[3];				/// pthread (server, client, stablizing)
pthread_mutex_t lock;			/// pthrea lock

int initNode(uint32_t nodeId, unsigned int fTime, bool menu);
void initServerSocket(uint16_t port);
void listenServerTCPSocket();
void listenServerUDPSocket();
void loopStablize();

void createReqPkt(char* buf, uint32_t targetId, uint32_t sId, char* ipAddr, 
					uint16_t port, int pktType);
void createResPkt(char* buf, uint32_t targetId, uint32_t sId, char* ipAddr, 
					uint16_t port, int pktType);
void createKeyTransPkt(char* buf, int size, uint32_t keys[], int keySize, int type);

int parse(char buf[], uint32_t* targetId, uint32_t* sId, char* ipAddr, 
			uint16_t* port, uint32_t keys[], int* keySize);
void sendReqClosestFingerPkt(int sockfd, uint32_t targetId, uint32_t sId, char* ipAddr, uint16_t port);
void sendReqSuccForPredPkt(int sockfd, uint32_t sId, char* sIpAddr, uint16_t sPort);
void sendReqSuccForSuccPkt(int sockfd, uint32_t sId, char* sIpAddr, uint16_t sPort);
void sendNotifyPkt(int sockfd, uint32_t sId, char* sIpAddr, uint16_t sPort,
					uint32_t id, char* ipAddr, uint16_t port);
void sendReqSuccForKeyPkt(int sockfd, uint32_t id, uint32_t sId, char* sIpAddr, uint16_t sPort);
void sendReqAlivePkt(int sockfd, char* ipAddr, uint16_t port);
int recvResPkt(int sockfd, uint32_t* sId, char* sIpAddr, uint16_t* sPort);

int connectToServer(int* sockfd, char* ipAddr, uint16_t port);
int readFromSocket(int sockfd, char* buf, unsigned int size);
int writeToSocket(int sockfd, char* buf, unsigned int size);
void printMenu();
#endif