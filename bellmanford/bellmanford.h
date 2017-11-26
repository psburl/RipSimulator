#ifndef BELLMAN_H
#define BELLMAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../improvc/improvc.h"
#include "../network/network.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#define MAX 100
#define INFINITE 1000
#define ERRORTRIES 1

typedef struct distanceVector{
    int used;
    int destination;
    int firstNode;
    int errors;
    double coust;
    double old;
}DistanceVector;


typedef struct dvTable{
    router_t origin;
    DistanceVector info[MAX][MAX];
}DvTable;

typedef struct dvMessage{
    int origin;
    int count;
    DistanceVector info[MAX];
}DvMessage;

DvTable getInitialRoutingTable(list_t* links, router_t* id);
void printDvTable(DvTable table);
DvMessage mountMessage(DvTable myTable,list_t* links, int destination, int poison);
DvTable updateMyTable(DvTable myTable, list_t* links, DvMessage message, int* updated);
DvTable updateErrorToSend(DvTable table, list_t* neighboors, int destination, int* updated);
#include "bellmanford.c"

#endif