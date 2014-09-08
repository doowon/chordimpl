/**@file chord.h
 *
 *  @author Doowon Kim
 *  @date Jul. 2014
 */

#ifndef CHORD_H
#define CHORD_H

#include "node.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h> 

#define DEFAULT_IP_ADDR "127.0.0.1"
#define DEFAULT_PORT 6000
#define DEFAULT_NODE_ID 0

Node* nd;
extern pthread_mutex_t lock;			/// pthread lock

void initChord(char* data, int dataSize, uint16_t port);
bool findSuccessor(unsigned char* targetId, unsigned char* sId, char* sIpAddr, uint16_t* sPort);
bool closestPrecedingFinger(unsigned char* targetId, unsigned char* sId, char* ipAddr, uint16_t* port);

void join();
void leave();

void askSuccForPred(unsigned char* sId, char* sIpAddr, uint16_t sPort,
					unsigned char* predId, char* predIpAddr, uint16_t* predPort);
void askSuccForSucc(unsigned char* sId, char* sIpAddr, uint16_t sPort,
					unsigned char* ssId, char* ssIpAddr, uint16_t* ssPort);
void askSuccForKeys(uint32_t id, uint32_t sId, char* sIpAddr, 
					uint16_t sPort, uint32_t keys[], int *keySize);
bool checkAlive(char* ipAddr, uint16_t port);

void getPredecesor(unsigned char* id, char* ipAddr, uint16_t* port);
void getSuccessor(unsigned char* id, char* ipAddr, uint16_t* port);
void getKeys(unsigned char* id, uint32_t keys[], int* num);
void setKeys(uint32_t keys[], int keySize);

void stabilize();
void notify(struct NodeInfo pNodeInf);
void buildSuccessorList();
void fixFingers();
void modifyPred(unsigned char* id, char* ipAddr, uint16_t port);

void cpyNodeInfo(struct NodeInfo* src, struct NodeInfo* dst);
bool between(const unsigned char* id, const unsigned char* start, 
				const unsigned char* end);

void printMenu();

/* For debug */
void printDebug();
void printFT();
void printSuccList();

#endif