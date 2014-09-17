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
bool findSuccessor(mpz_t targetId, mpz_t sId, char* sIpAddr, uint16_t* sPort);
bool closestPrecedingFinger(mpz_t targetId, mpz_t sId, char* ipAddr, uint16_t* port);

void join();
void leave();

void askSuccForPred(mpz_t sId, char* sIpAddr, uint16_t sPort,
					mpz_t predId, char* predIpAddr, uint16_t* predPort);
void askSuccForSucc(mpz_t sId, char* sIpAddr, uint16_t sPort,
					mpz_t ssId, char* ssIpAddr, uint16_t* ssPort);
void askSuccForKeys(uint32_t id, uint32_t sId, char* sIpAddr, 
					uint16_t sPort, uint32_t keys[], int *keySize);
bool checkAlive(char* ipAddr, uint16_t port);

void getPredecesor(mpz_t id, char* ipAddr, uint16_t* port);
void getSuccessor(mpz_t id, char* ipAddr, uint16_t* port);
void getKeys(mpz_t id, uint32_t keys[], int* num);
void setKeys(uint32_t keys[], int keySize);

void stabilize();
void notify(struct NodeInfo pNodeInf);
void buildSuccessorList();
void fixFingers();
void modifyPred(mpz_t id, char* ipAddr, uint16_t port);

void cpyNodeInfo(struct NodeInfo* src, struct NodeInfo* dst);
void printMenu();

/* For debug */
void printDebug();
void printFT();
void printSuccList();

#endif