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
#define DEFAULT_PORT 10000
#define DEFAULT_NODE_ID 0

Node* nd;
pthread_mutex_t lock;					/// pthread lock
mpz_t max; mpz_t min;					/// max and min for comparing Keys

mpz_t tmp;
mpz_t tmp2;

void initChord(mpz_t id, FILE* keysfp, uint16_t port);
bool findSuccessor(mpz_t targetId, mpz_t sId, char* sIpAddr, uint16_t* sPort);
bool closestPrecedingFinger(mpz_t targetId, mpz_t sId, char* ipAddr, uint16_t* port);

void join();
void leave();

void askSuccForPred(mpz_t sId, char* sIpAddr, uint16_t sPort,
					mpz_t predId, char* predIpAddr, uint16_t* predPort);
void askSuccForSucc(mpz_t sId, char* sIpAddr, uint16_t sPort,
					mpz_t ssId, char* ssIpAddr, uint16_t* ssPort);
void askSuccForKey(mpz_t id, char* sIpAddr, uint16_t sPort, unsigned char* data, int* dataSize);
bool checkAlive(char* ipAddr, uint16_t port);

void getPredecesor(mpz_t id, char* ipAddr, uint16_t* port);
void getSuccessor(mpz_t id, char* ipAddr, uint16_t* port);
int getData(unsigned char* data);
void getKeys(mpz_t id, mpz_t keys[], int keySize, char* data[]);
void setKeys(mpz_t keys[], int keySize);

void stabilize();
void notify(const mpz_t);
void buildSuccessorList();
void fixFingers();
void modifyPred(mpz_t id, char* ipAddr, uint16_t port);

void cpyNodeInfo(struct NodeInfo* src, struct NodeInfo* dst);
void printMenu();

/* For debug */
void printDebug();
void printFT();
void printSuccList();

// For simulation
uint32_t sim_findSuccessor(mpz_t targetId, mpz_t sId, char* sIpAddr, uint16_t* sPort);

#endif