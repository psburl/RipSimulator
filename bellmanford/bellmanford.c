DvTable getInitialRoutingTable(list_t* links, router_t* router){

    DvTable table;
    table.origin = *router;

    int i, j;
    for(i = 0; i < MAX; i++)
        for(j=0; j< MAX; j++)
            table.info[i][j].used=0;

    node_t* node = links->head;
    list_t* neighboors = new_list(sizeof(int));
    while(node != NULL){

        link_t* link = (link_t*)(node->data);
        //it's router neighboor
		if(link->router1 == router->id || link->router2 == router->id){
            DistanceVector vector;
            vector.used=1;
			vector.firstNode = link->router1 == router->id ? link->router2: link->router1;
			vector.destination = vector.firstNode;
			vector.coust = link->coust;
            table.info[router->id][vector.destination] = vector; 

            DistanceVector neighboor;
            neighboor.used=1;
            neighboor.firstNode = router->id;
            neighboor.destination = router->id;
            neighboor.coust = INFINITE;
            table.info[vector.destination][router->id] = neighboor;

            list_append(neighboors, &vector.destination);
        }
        node = node->next;
    }

    node = links->head;
    while(node != NULL){
        link_t* link = (link_t*)(node->data);
		if(link->router1 == router->id || link->router2 == router->id){
            int id = link->router1 == router->id ? link->router2: link->router1;
            node_t* element = neighboors->head;
            while(element != NULL){
                int neighboor = *(int*)element->data;
                if(neighboor != id){
                    DistanceVector vector;
                    vector.used=1;
                    vector.firstNode = neighboor;
                    vector.destination = neighboor;
                    vector.coust = INFINITE;
                    table.info[id][neighboor] = vector; 
                }
                element = element->next;
            }
        }
        node = node->next;
    }

    DistanceVector vector;
    vector.used=1;
    vector.firstNode = table.origin.id;
    vector.destination = vector.firstNode;
    vector.coust = 0;
    table.info[table.origin.id][table.origin.id] = vector;
	
	return table;
}

void printDvTable(DvTable table){

    int i, j;
    for(i = 0; i < MAX; i++){

        if(i != table.origin.id)
            continue;

        for(j=0; j< MAX; j++){
            
            DistanceVector vector = table.info[i][j];

            if(vector.used == 1){
                printf("router %d ", i);
                if(vector.coust == INFINITE)
                    printf(" has distance INFINITE to %d\n", j);
                else{
                    printf(" has distance %8.2lf to %d ", vector.coust, j);
                    printf(" starting by %d\n", vector.firstNode);
                }
            }
        }
    }
}

DvMessage mountMessage(DvTable myTable,list_t* links, int destination, int poison){
    
    DvMessage message;
    message.count=0;
    int myId = myTable.origin.id;
    message.origin = myId;
    node_t* node = links->head;
    while(node != NULL){

        link_t* link = (link_t*)(node->data);
        int nId = link->router1 == myId ? link->router2: link->router1;
        DistanceVector vector = myTable.info[myId][nId];
        memcpy(&message.info[message.count],&vector, sizeof(vector));
        if(poison && destination == vector.firstNode)
            message.info[message.count].coust = INFINITE;
        message.info[message.count].firstNode = -1;
        message.count++;
        node = node->next;
    }
    return message;
}

DvTable updateMyTable(DvTable myTable, list_t* links, DvMessage message, int* updated){

    int i;
    for(i =0;i<message.count;i++){

        DistanceVector vector = message.info[i];
        DistanceVector myVector = myTable.info[myTable.origin.id][vector.destination];
        int sum = vector.coust + myTable.info[myTable.origin.id][message.origin].coust;
        if(myVector.used == 1){
            if(myVector.coust > sum){
                *updated = 1;
                myVector.firstNode = message.origin;
                myVector.coust = sum;
            }
        }
        else{
            *updated = 1;
            myVector.used = 1;
            myVector.destination = vector.destination;
            myVector.firstNode = message.origin;
            myVector.coust = sum;
            if(sum > INFINITE)
                myVector.coust = INFINITE;

            node_t* node = links->head;
            while(node != NULL){

                link_t* link = (link_t*)(node->data);
                int nId = link->router1 == myTable.origin.id ? link->router2: link->router1;
                DistanceVector column;
                column.used = 1;
                column.destination = vector.destination;
                column.firstNode = message.origin;
                column.coust = INFINITE;
                myTable.info[nId][vector.destination] = column;
                node = node->next;
            }
        }

        myTable.info[myTable.origin.id][vector.destination] = myVector;
        myTable.info[message.origin][vector.destination] = vector;
    }

    
    return myTable;
}