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
    printf("---------");
    for(i = 0; i < MAX; i++)
            if(table.info[table.origin.id][i].used)
                printf("----------------");
    printf("\n\t|\t");
    for(i = 0; i < MAX; i++)
            if(table.info[table.origin.id][i].used)
                printf("%d\t|\t", table.info[table.origin.id][i].destination);
    printf("\n");
    printf("---------");
    for(i = 0; i < MAX; i++)
            if(table.info[table.origin.id][i].used)
                printf("----------------");
    printf("\n");
    
    
     for(i = 0; i < MAX; i++){
        if(i != table.origin.id)
            continue;

        printf("%d\t|", i);
        int use=0;
        for(j=0; j< MAX; j++){
            DistanceVector vector = table.info[i][j];
            use = vector.used;
            if(use == 1){
                if(vector.coust == INFINITE)
                    printf("INFINITE\t|");
                else{
                    printf("%6.2lf(%d)\t|", vector.coust, vector.firstNode);
                }
            }
        }
        if(use){
            printf("\n");
            printf("---------");
            int k;
            for(k = 0; k < MAX; k++)
                    if(table.info[table.origin.id][k].used)
                        printf("----------------");
            printf("\n");
        }
    }
    printf("\n");
    printf("---------");
    int k;
    for(k = 0; k < MAX; k++)
            if(table.info[table.origin.id][k].used)
                printf("----------------");
    printf("\n");
}

DvMessage mountMessage(DvTable myTable,list_t* links, int destination, int poison){
    
    DvMessage message;
    message.count=0;
    int myId = myTable.origin.id;
    message.origin = myId;
    message.info[message.count].coust = 0;
    message.info[message.count].destination = myId;
    message.count++;

    int j;
    for(j=0; j< MAX; j++){
        if(myTable.info[myId][j].used == 1){

            DistanceVector vector = myTable.info[myId][j];
            memcpy(&message.info[message.count],&vector, sizeof(vector));
            if(poison && destination == vector.firstNode)
                message.info[message.count].coust = INFINITE;
            message.info[message.count].destination = j;
            message.count++;
        }  
    }

    return message;
}

DvTable updateMyTable(DvTable myTable, list_t* links, DvMessage message, int* updated){

    int i;
    node_t* node = links->head;
    while(node != NULL){
        link_t* link = (link_t*)(node->data);
        if(link->router1 == message.origin || link->router2 == message.origin)
            link->dead = 0;
        node = node->next;
    }

    int coustToThis =  myTable.info[myTable.origin.id][message.origin].coust;
    for(i =0;i<message.count;i++){

        DistanceVector vector = message.info[i];
        DistanceVector myVector = myTable.info[myTable.origin.id][vector.destination];
        memcpy(&message.info[message.origin],&vector, sizeof(vector));
        int sum = vector.coust + coustToThis;
        if(sum >= INFINITE && message.origin == vector.destination){
            int j;
            for(j =0;j<message.count;j++){
                if(message.info[j].destination == myTable.origin.id)
                    sum = message.info[j].coust;
            }
        }   
        
        if(myVector.used == 1){
            if(myVector.coust > sum){
                *updated = 1;
                myVector.firstNode = message.origin;
                myVector.coust = sum;
            }
            if(myVector.firstNode == message.origin){
                if(myVector.coust != sum){
                    *updated = 1;
                    myVector.coust = INFINITE;
                    
                    node_t* node = links->head;
                    while(node != NULL){

                        link_t* link = (link_t*)(node->data);
                        int nId = link->router1 ==myTable.origin.id ? link->router2: link->router1;
                        
                        if(myTable.info[nId][message.origin].coust < myTable.info[myTable.origin.id][message.origin].coust){
                            myTable.info[myTable.origin.id][message.origin].coust = myTable.info[nId][message.origin].coust + coustToThis;
                            myTable.info[myTable.origin.id][message.origin].firstNode = nId;
                        }
                        
                        node = node->next;
                    }
                }
            }
        }
        else{
            *updated = 1;
            myVector.used = 1;
            myVector.destination = vector.destination;
            myVector.firstNode = myTable.info[myTable.origin.id][message.origin].firstNode;
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

    for(i =0;i<MAX;i++){
        DistanceVector vector = myTable.info[myTable.origin.id][i];
        if(vector.used == 0)
            continue;
        if(vector.coust == INFINITE){
            int j;
            for(j =0;j<MAX;j++){
                DistanceVector other = myTable.info[myTable.origin.id][j];
                if(other.used == 0)
                    continue;
                if(other.firstNode == i && other.firstNode != other.destination){
                    other.coust = INFINITE;
                    other.firstNode = -1;
                    myTable.info[myTable.origin.id][j] = other;

                    node_t* node = links->head;
                    while(node != NULL){
                        link_t* link = (link_t*)(node->data);
                        if(link->router1 == myTable.origin.id || link->router2 == myTable.origin.id){
                            int id = link->router1 == myTable.origin.id ? link->router2: link->router1;
                            if(id == j){
                                DistanceVector vector = myTable.info[myTable.origin.id][id];
                                if(vector.coust > link->coust){
                                    myTable.info[myTable.origin.id][id].coust = link->coust;
                                    myTable.info[myTable.origin.id][id].firstNode = id;
                                }
                            }
                        }
                        node = node->next;
                    }
                }
             }
        }
    }
    
    return myTable;
}

DvTable updateErrorToSend(DvTable table, list_t* neighboors, int destination, int* updated){

    int id = table.origin.id;

    if(table.info[id][destination].errors < ERRORTRIES-1)
        table.info[id][destination].errors++;
    else{
        table.info[id][destination].errors = 0;
        
        *updated = 1;

        table.info[id][destination].coust = INFINITE;
        table.info[destination][id].coust = INFINITE;

        node_t* node = neighboors->head;
        while(node != NULL){

            link_t* link = (link_t*)(node->data);
            int nId = link->router1 == id ? link->router2: link->router1;
            
            if(nId != destination){
                double sum = link->coust + table.info[nId][destination].coust;
                if(table.info[id][destination].coust > sum){
                    table.info[id][destination].coust = sum;
                    table.info[id][destination].firstNode = nId;
                }                   
            }
            node = node->next;
        }
    }

    return table;
}