/** @file node.h
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#ifndef NODE_H
#define NODE_H

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

#define DEBUG 0

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

int listenfd; 					/// socket for listener(server)
int connfd; 					/// socket for listener
int connfdCli; 					/// socket for client

struct sockaddr_in servAdr; 	/// server address
struct sockaddr_in servAdrCli; 	/// server address for client

char sendBufServ[16];			/// send buffer for server
char recvBufServ[16];			/// recv buffer for server (8byte)

char recvBufCli[16];			/// recv buffer for client

pthread_t tid[2];				/// pthread (server, client, stablizing)
pthread_mutex_t lock;			/// pthrea lock

int initNode(uint32_t nodeId, unsigned int fTime);
void pthreadJoin();
void initServerSocket();
int listenServerSocket();
int connectToServer(char* ipAddr, uint16_t port);
int checkConnection(char* ipAddr, uint16_t port);
int readFromSocket(int fd, char* buf);
int writeToSocket(int fd, char* buf);
void loopStablize();
int closeSocket(int socketfd);
void createReqPkt(char* buf, uint32_t targetId, uint32_t successorId, 
					char* ipAddr, uint16_t port, int res);
void createResPkt(char* buf, uint32_t targetId, uint32_t successorId, 
					char* ipAddr, uint16_t port, int res);
void createKeyResPkt(char* buf, uint32_t keys[], int num);
int parse(char* buf, uint32_t* targetId, uint32_t* successorId, 
					char* ipAddr, uint16_t* port);
int parseForKeyResPkt(char* buf, uint32_t keys[], int* num);
int sendReqPkt(uint32_t targetId, uint32_t successorId, 
						char* ipAddr, uint16_t port);
int recvResPkt(uint32_t targetId, uint32_t* successorId, 
						char* ipAddr, uint16_t* port);
int recvKeyResPkt(uint32_t keys[], int* num);
int sendAskSuccForPredPkt(uint32_t sId, char* sIpAddr, uint16_t sPort);
int sendAskSuccForSuccPkt(uint32_t sId, char* sIpAddr, uint16_t sPort);
int sendAskSuccForKeyPkt(uint32_t id, uint32_t sId, char* sIpAddr, uint16_t sPort);
int sendNotifyPkt(uint32_t sId, char* sIpAddr, uint16_t sPort,
					uint32_t id, char* ipAddr, uint16_t port);
#endif