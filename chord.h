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
#define DEFAULT_NODE_ID 1

Node* nd;
extern pthread_mutex_t lock;			/// pthread lock

int initChord(uint32_t nodeId);
bool findSuccessor(uint32_t targetId, uint32_t* sId, char* sIpAddr, uint16_t* sPort);
bool closestPrecedingFinger(uint32_t targetId, uint32_t* sId, char* ipAddr, uint16_t* port);

void join();
void leave();

void askSuccForPred(uint32_t sId, char* sIpAddr, uint16_t sPort,
					uint32_t* predId, char* predIpAddr, uint16_t* predPort);
void askSuccForSucc(uint32_t sId, char* sIpAddr, uint16_t sPort,
					uint32_t* ssId, char* ssIpAddr, uint16_t* ssPort);
void askSuccForKeys(uint32_t id, uint32_t sId, char* sIpAddr, 
					uint16_t sPort, uint32_t keys[], int *keySize);
bool checkAlive(char* ipAddr, uint16_t port);

void getPredecesor(uint32_t* id, char* ipAddr, uint16_t* port);
void getSuccessor(uint32_t* id, char* ipAddr, uint16_t* port);
void getKeys(uint32_t id, uint32_t keys[], int* keySize);
void setKeys(uint32_t keys[], int keySize);

void stabilize();
void notify(struct NodeInfo pNodeInf);
void buildSuccessorList();
void fixFingers();
void modifyPred(uint32_t id, char* ipAddr, uint16_t port);
int transferKeys(uint32_t id, char* ipAddr, uint16_t port, uint32_t keys[], int keySize);

void cpyNodeInfo(struct NodeInfo* src, struct NodeInfo* dst);
int cmpfunc(const void* a, const void* b);

void printMenu();

/* For debug */
void printDebug();
void printFT();
void printSuccList();

#endif