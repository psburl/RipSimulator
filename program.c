#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "improvc/improvc.h"
#include "network/network.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "bellmanford/bellmanford.h"

#define BUFLEN 100  //Max length of buffer
#define SocketAddress struct sockaddr_in
#define TIMEOUT 5
#define TRIES 3
#define TYPE_MESSAGE 1
#define TYPE_TABLE 2

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
typedef struct MessageData{
	
	int routerId;
	char message[BUFLEN];
	int type;
	DvMessage dvMessage;

}MessageData;

typedef struct receiverArg{
	
	router_t* router;
	list_t* routers;
	list_t* neighboors;
	MessageData data;

}ReceiverArg;

typedef struct senderArg{
	
	list_t* routers;
	list_t* neighboors;
}SenderArg;

DvTable table;

void send_to_next(void* senderArg, MessageData* data);

void updateTable(DvTable newTable){
	pthread_mutex_lock(&mutex);
	table = newTable;
	pthread_mutex_unlock(&mutex);
}

MessageData* NewMessageData(){

	MessageData* data = malloc(sizeof(MessageData));
	return data;
}

MessageData* GetMessage(){

	MessageData* data = NewMessageData();
	printf("type router name to send message: ");
	scanf("%d", &(data->routerId));
	getchar();
	printf("type message to send: ");
	fflush(stdout);
	fflush(stdin);
	fgets(data->message, BUFLEN, stdin);
	data->type = TYPE_MESSAGE;
	return data;
}

void die(char *s){
    perror(s);
    exit(1);
}

router_t* getRouter(list_t* routers, int id){

	node_t* element = routers->head;
	while(element!=NULL){

		router_t* router = (router_t*)(element->data);
		if(router->id == id)
			return router;

		element = element->next;
	}
	return NULL;
}

void resend(list_t* routers, list_t* neighboors, int id){

	printf("resending table(updated)...\n");	
	node_t* node = neighboors->head;
	while(node != NULL){

		link_t* link = (link_t*)(node->data);
		int nId = link->router1 == id ? link->router2: link->router1;
		DvMessage message = mountMessage(table, neighboors, nId, 1);
		SenderArg arg;
		arg.routers = routers;
		arg.neighboors = neighboors;
		MessageData messageData;
		messageData.routerId = nId;
		messageData.dvMessage = message;
		messageData.type = TYPE_TABLE;

		send_to_next(&arg, &messageData);

		node = node->next;
	}
}

void* updateWhenError(void* senderArg){

	SenderArg* arg = (SenderArg*)senderArg;
	list_t* routers = arg->routers;
	list_t* neighboors = arg->neighboors;
	resend(routers, neighboors, table.origin.id);
	return 0;
}

void send_to_next(void* senderArg, MessageData* data){

	SenderArg* arg = (SenderArg*)senderArg;
	list_t* routers = arg->routers;
	
	int node = table.info[table.origin.id][data->routerId].firstNode;

	if(data->type == TYPE_TABLE)
		node = data->routerId;

	if(table.info[table.origin.id][data->routerId].used == 0){
		printf("sender: unreacheable\n");
		return;
	}

	if(table.info[table.origin.id][data->routerId].coust == INFINITE){
		printf("sender: unreacheable\n");
		return;
	}

	router_t* neighboor = getRouter(routers, node);

	if(neighboor == NULL){
		printf("sender: unreacheable\n");
		return;
	}

	SocketAddress socketAddress;
    int socketId;
	socklen_t slen = sizeof(socketAddress);
	 
    if ((socketId=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        die("sender: Error getting socket\r\n");
    
    memset((char *) &socketAddress, 0, sizeof(socketAddress));
    socketAddress.sin_family = AF_INET;

	
	// send message to correct port
	socketAddress.sin_port = htons(neighboor->port);
 	// set ip to destination ip
	if (inet_aton(neighboor->ip , &socketAddress.sin_addr) == 0) {
    	fprintf(stderr, "sender: inet_aton() failed\n");
    	exit(1);
	}
	printf("sender: sending packet to router %d\n", neighboor->id);

	int tries = TRIES;
	int ack = 0;
	while(tries--){

		if (sendto(socketId, data, sizeof(data)+(BUFLEN)+sizeof(DistanceVector)*MAX , 0 , (struct sockaddr *) &socketAddress, slen)==-1)
			die("sender: sendto()\n");
				
		struct timeval read_timeout;
		read_timeout.tv_sec = TIMEOUT;
		read_timeout.tv_usec = 10;
		
		setsockopt(socketId, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
		
		if (recvfrom(socketId, &ack, sizeof(ack), 0, (struct sockaddr *) &socketAddress, &slen) == -1)
			printf("Error send package, retrying\n");
		
		if(ack)
			break;
	}
	if(!ack){
		printf("It's not possible to send package\n");
		int updated;
		list_t* neighboors = arg->neighboors;
		DvTable newTable = updateErrorToSend(table, neighboors, neighboor->id, &updated);
		if(updated){
			updateTable(newTable);
			printDvTable(newTable);
			updateWhenError(arg);
		}
	}
}

void* send_table(void* argReceiver){
	
	ReceiverArg* arg = (ReceiverArg*)argReceiver;
	int id = arg->router->id;
	list_t* routers = arg->routers;
	list_t* neighboors = arg->neighboors;

	while(1){
		printf("resending table...\n");
		node_t* node = neighboors->head;
		while(node != NULL){
			
			link_t* link = (link_t*)(node->data);
			int nId = link->router1 == id ? link->router2: link->router1;
			
			DvMessage message = mountMessage(table, neighboors, nId, 1);
			SenderArg arg;
			arg.routers = routers;
			arg.neighboors = neighboors;
			MessageData messageData;
			
			messageData.routerId = nId;
			messageData.dvMessage = message;
			messageData.type = TYPE_TABLE;
			send_to_next(&arg, &messageData);
			node = node->next;
		}
		printf("table refresh finished..waiting\n");
		sleep (5+(id*3));
	}

	return 0;
}

void* call_sender(void* arg_router){

    while(1){
		MessageData* data = GetMessage();
		send_to_next(arg_router, data);
    }	
    return 0;
}

void* makeUpdate(void* argReceiver){
	ReceiverArg* arg = (ReceiverArg*)argReceiver;
	int updated = 0;
	DvTable newTable = updateMyTable(table, arg->neighboors, arg->data.dvMessage, &updated);
	updateTable(newTable);
	printDvTable(table);
	if(updated){
		int id = arg->router->id;
		list_t* routers = arg->routers;
		list_t* neighboors = arg->neighboors;
		resend(routers, neighboors, id);
	}
	return 0;
}

void* call_receiver(void* argReceiver){

	ReceiverArg* arg = (ReceiverArg*)argReceiver;
	router_t* router = arg->router;
	
    SocketAddress currentSocket, destinationSocket;     
    int socketId, receivedDataLength;
    socklen_t slen = sizeof(destinationSocket);
     
    if ((socketId=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        die("receiver: socket\n");
     
    // zero out the structure
    memset((char *) &currentSocket, 0, sizeof(currentSocket));
     
    currentSocket.sin_family = AF_INET;
    currentSocket.sin_port = htons(router->port);
    currentSocket.sin_addr.s_addr = htonl(INADDR_ANY);
     
    if(bind(socketId , (struct sockaddr*)&currentSocket, sizeof(currentSocket)) == -1)
        die("receiver: bind\n");

    while(1){

    	MessageData* data = NewMessageData();
        fflush(stdout);

        if ((receivedDataLength = recvfrom(socketId, data, sizeof(data)+(BUFLEN)+sizeof(DistanceVector)*MAX, 0, (void*)&destinationSocket, &slen)) == -1)
            die("receiver: recvfrom()\n");
         
        printf("\nreceiver: Received packet from %s:%d\n", inet_ntoa(destinationSocket.sin_addr), ntohs(destinationSocket.sin_port));
        
		if(data->type == TYPE_MESSAGE){
			if(data->routerId == router->id)
				printf("Data: %s\n" , data->message);
			else{
				printf("package will be sent to router: %d\n" , data->routerId);
				SenderArg sender;
				sender.neighboors = arg->neighboors;
				sender.routers = arg->routers;
				send_to_next(&sender, data);
			}
		}
		else{
			arg->data = *data;
			pthread_t thread; 
			pthread_create(&thread, NULL, makeUpdate, arg);
		}
        
        int ack = 1;
        if (sendto(socketId, &ack, sizeof(ack), 0, (void*)&destinationSocket, slen) == -1)
            die("receiver: sendto()\n");
    }
 
    close(socketId);
    return 0;
}

void start_operation(SenderArg* sender, ReceiverArg* receiver){
	pthread_t threads[3];
	pthread_create(&(threads[0]), NULL, call_receiver, receiver);
	pthread_create(&(threads[2]), NULL, send_table, receiver);
	pthread_create(&(threads[1]), NULL, call_sender, sender);
	pthread_join(threads[1], NULL);
	pthread_join(threads[2], NULL);
	pthread_join(threads[0], NULL);
}

void print_routingTable(void* v){

	graph_path_t* path = (graph_path_t*)v;
	printf("weight %8.2lf ", path->distance);
	if(path->to != NULL)
		printf("to destination %d ",  *(int*)path->to->data);
	if(path->start != NULL)
		printf("starting by: %d\n",  *(int*)path->start->data);
	else
		printf("unreachable\n");
}

void print_init(router_t* router){
	printf("Router info...\n");
	printf("----------------------------------------------\n");
	printf("Router   id: %d\n", router->id);
	printf("Router port: %d\n", router->port);
	printf("Router   ip: %s\n" , router->ip);
	printf("----------------------------------------------\n");
	printf("Loading routing table...\n");
	printf("----------------------------------------------\n");
	printf("----------------------------------------------\n");
}

int main(int argc, char **argv){

	int router_id = atoi(argv[1]);
	printf("Starting router %d\n", router_id);

	list_t* routers = read_routers();
	list_t* neighboors = read_links(router_id);
	router_t* router = getRouter(routers, router_id);

	table = getInitialRoutingTable(neighboors, router);
	printDvTable(table);

	SenderArg arg;
	arg.routers = routers;
	arg.neighboors = neighboors;
	ReceiverArg rec;
	rec.router  = router;
	rec.neighboors = neighboors;
	rec.routers = routers;
	
	start_operation(&arg, &rec);
	
	return 0;
}