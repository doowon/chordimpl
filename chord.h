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

#define DEBUG 1

typedef int bool;
#define true 1
#define false 0

#define IP_ADDR "127.0.0.1"
#define DEFAULT_PORT 6000

Node* nd;

int initChordNode(uint32_t nodeId);
int findSuccessor(uint32_t targetId, uint32_t* successorId, 
						char* ipAddr, uint16_t* port);
int findInitSuccessor(uint32_t targetId, uint32_t* successorId,
						char* ipAddr, uint16_t* port);
int closestPrecedingFinger(uint32_t targetId, uint32_t* successorId, 
								char* ipAddr, uint16_t* port);
int join();
void askSuccforPred(uint32_t sId, char* sIpAddr, uint16_t sPort,
					uint32_t* predId, char* predIpAddr, uint16_t* predPort);
void getPredecesor(uint32_t* id, char* ipAddr, uint16_t* port);
void stabilize();
int notify(struct NodeInfo pNodeInf);
void fixFingers();
void modifyPred(uint32_t id, char* ipAddr, uint16_t port);
void printDebug();
void printFT();

#endif