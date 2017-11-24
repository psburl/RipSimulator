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
    printf("router %d....\n", table.origin.id);
    for(i = 0; i < MAX; i++){

        DistanceVector line = table.info[table.origin.id][i];
        if(line.used == 0)
            continue;

        printf("|%d|->", i);

        for(j=0;j<MAX;j++){
            
            DistanceVector vector = table.info[i][j];
            if(vector.used == 0){
                continue;
            }

            if(vector.coust>=INFINITE)
                printf("%d-INFINITE\t|",j);
            else 
                printf("%d-%4.2lf(%d)\t|", j, vector.coust, vector.firstNode);
        }            
        printf("\n");
    }
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

    printf("SEND TO %d\n",destination);
    for(j=0;j<message.count;j++){
        
        DistanceVector vector = message.info[j];
        if(vector.used == 0){
            continue;
        }

        if(vector.coust>=INFINITE)
            printf("%d-INFINITE--|", vector.destination);
        else 
            printf("%d-%4.2lf(%d)--|", vector.destination, vector.coust, vector.firstNode);
    } 

    return message;
}

DvTable updateMyTable(DvTable myTable, list_t* links, DvMessage message, int* updated){

    int j;
    printf("RECEIVE FROM %d\n", message.origin);
    for(j=0;j<message.count;j++){
        
        DistanceVector vector = message.info[j];
        if(vector.used == 0){
            continue;
        }

        if(vector.coust>=INFINITE)
            printf("%d-INFINITE--|", vector.destination);
        else 
            printf("%d-%4.2lf(%d)--|", vector.destination, vector.coust, vector.firstNode);
    }            

    int i;
    for(i = 0; i < message.count; i++){

        int destination = message.info[i].destination;

        myTable.info[myTable.origin.id][destination].used = 1;

        if(myTable.info[message.origin][destination].used ==0){
            *updated = 1;
            node_t* node = links->head;
            while(node != NULL){
                link_t* link = (link_t*)(node->data);
                int nId = link->router1 == myTable.origin.id ? link->router2: link->router1;
                if(myTable.info[nId][destination].used == 0){
                    myTable.info[nId][destination].used = 1;
                    myTable.info[nId][destination].coust = INFINITE;
                }
                node = node->next;
            }
        }
        
        myTable.info[message.origin][destination].coust =  message.info[i].coust;
        myTable.info[message.origin][destination].destination = message.info[i].destination;
    }

    for(i = 0; i < MAX; i++){

        myTable.info[message.origin][i].coust =  message.info[i].coust;
        myTable.info[message.origin][i].destination = message.info[i].destination;

        myTable.info[myTable.origin.id][i].old =  myTable.info[myTable.origin.id][i].coust;
        myTable.info[myTable.origin.id][i].coust = INFINITE;
        if(i == myTable.origin.id)
             myTable.info[myTable.origin.id][i].coust = 0.00;
    }

    for(i = 0; i < MAX; i++){

        if(i == myTable.origin.id)
            continue;

        double coustToThis = INFINITE;
        node_t* node = links->head;
        while(node != NULL){
            link_t* link = (link_t*)(node->data);
            int nId = link->router1 == myTable.origin.id ? link->router2: link->router1;
            if(nId == i)
                coustToThis =  link->coust;

            node = node->next;
        }

        int j;
        for(j=0; j < MAX; j++){

            if(myTable.info[i][j].used == 0)
                continue;

            DistanceVector vector = myTable.info[i][j];

            double sum = coustToThis + vector.coust;
            if(myTable.info[myTable.origin.id][j].coust > sum){
                myTable.info[myTable.origin.id][j].coust = sum;
                myTable.info[myTable.origin.id][j].firstNode = i;
                myTable.info[myTable.origin.id][j].destination = j;
            }
        }
    }

    for(i = 0; i < MAX; i++){

        if(myTable.info[myTable.origin.id][i].used == 0)
            continue;

        if(myTable.info[myTable.origin.id][i].coust !=
        myTable.info[myTable.origin.id][i].old)
            *updated = 1;
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
        
        int i;
        for(i = 0; i < MAX; i++)
            table.info[destination][i].used = 0;
        
        node_t* node = neighboors->head;
        while(node != NULL){

            link_t* link = (link_t*)(node->data);
            int nId = link->router1 == id ? link->router2: link->router1;
            if(table.info[nId][id].used == 1 && 
                table.info[nId][destination].used == 1 && nId != destination){
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