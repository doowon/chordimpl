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

#define FT_SIZE 3

#define DEBUG 1

struct _Node;
typedef struct _Node Node;

struct NodeInfo{
	uint32_t id;
	char ipAddr[15];
	uint16_t port;
};

struct FingerTable{
	int start; ///(n + 2^(k-1))
	struct NodeInfo successorInfo;
};

struct _Node{
	struct NodeInfo ndInfo;
	int key[10]; 				/// Array of keys
	int keySize; 				/// the number of keys
	struct NodeInfo predInfo;
	struct FingerTable ft[FT_SIZE];
	int ftSize;
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

int initNode(uint32_t nodeId);
void initServerSocket();
int listenServerSocket();
int connectToServer(char* ipAddr, uint16_t port);
int readFromSocket(int fd, char* buf);
int writeToSocket(int fd, char* buf);
void loopStablize();
int closeSocket(int socketfd);
int createReqPkt(char* buf, uint32_t targetId, uint32_t successorId, 
					char* ipAddr, uint16_t port, int res);
int createResPkt(char* buf, uint32_t targetId, uint32_t successorId, 
					char* ipAddr, uint16_t port, int res);
int parse(char* buf, uint32_t* targetId, uint32_t* successorId, 
					char* ipAddr, uint16_t* port);
int sendReqPkt(uint32_t targetId, uint32_t successorId, 
						char* ipAddr, uint16_t port);
int recvResPkt(uint32_t targetId, uint32_t* successorId, 
						char* ipAddr, uint16_t* port);
int sendAskPkt(uint32_t sId, char* sIpAddr, uint16_t sPort,
				uint32_t* predId, char* predIpAddr, uint16_t* predPort);
int sendNotifyPkt(uint32_t sId, char* sIpAddr, uint16_t sPort,
					uint32_t id, char* ipAddr, uint16_t port);
#endif