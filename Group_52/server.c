//Server Side Programming
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>

void reverse(char *x, int begin, int end){
   char c;
   if (begin >= end)
      return;

   c= *(x+begin);
   *(x+begin) = *(x+end);
   *(x+end)   = c;
   reverse(x, ++begin, --end);
}

//function to initialise the server
void serverInit(struct sockaddr_in serverAddress){
    // initializes the serverAddress with all 0's
    memset(&serverAddress,0,sizeof(serverAddress));
}


void itoa(int num, char* s){ 
    int i = 0; 
    int f = 0;
    if(num == 0){ 
        s[i++] = '0'; 
        s[i] = '\0'; 
    }
    if(num<0){ 
        f =1;
        num = -num; 
    } 
    while(num){ 
        int rem = num%10; 
        s[i++] = (rem>9)?(rem-10)+'a':rem+'0'; 
        num = num/10; 
    } 
    if(f)s[i++] = '-'; 
    s[i] = '\0';
    reverse(s,0,i-1); 
  
} 


// Max out_buff side of the client
#define MAX_LINE 1024
#define MAX_CLIENTS 100 // Max number of clients allowed for the server.
int clients[MAX_CLIENTS]; //array to store clients
int tot_cost[MAX_CLIENTS]; //array to store total cost for each client


// signal handler to control the action of server down by pressing CTRL + C
void ctrlchandler(int num){
	char out_buff[MAX_LINE];
	for(int i=0; i<MAX_CLIENTS; i++){
		if(clients[i] != -1){
			// Sending message to client
			memset(out_buff,0,MAX_LINE);	
			char *temp = "2 Server Down";
			int k = 0;
			for(int z= 0; temp[z]; z++){
				out_buff[k++] = temp[z];
			}
			out_buff[k] = '\0';
			write(clients[i] , out_buff , sizeof(out_buff));

			if(close(clients[i]) < 0){
				printf("Socket could not be released\n");
			}
			clients[i] = -1;	
            printf("Server Exited Gracefully\n" );
		}
	}
	
	exit(0);
}
void funn(int f){
    if(f){
        printf("Server side programming\n");
    }
    else{
        printf("Client side Programming\n");
    }
}

// funciton to check if we can accomodate this client and if yes then answer all its queries
void func(int clientSocket){
    int f = 0;
    int j = -1; //variable to find the first empty spot in the clients array
    //checking if there is space to accomodate this client
    for(int i=0; i<MAX_CLIENTS; i++){
        if(clients[i] == clientSocket){ // i.e., client is already present in the list
            j=i;
            f = 1;
            break;
        } 
        if(clients[i] == -1){
            j = i;
            break;
        }

    }
    if(!f){
        if(j == -1){ // no space to add more cients
            printf("No more space\n");
            return;
        }
        clients[j] = clientSocket;
    }

    printf("Connection Successful\n");
    //now responding to the queries from client side
    char in_buff[MAX_LINE];
    char out_buff[MAX_LINE];
    while(1){
        memset(in_buff,0,MAX_LINE); //initialising the buffers with all 0's
        memset(out_buff,0,MAX_LINE);
        //reading data from client
        read(clientSocket, in_buff, sizeof(in_buff));
        
        char reqType = in_buff[0];

        // if client wants to exit
        if(reqType == '1'){
            // freeing this client
            clients[j] = -1;
            
            
            out_buff[0] = '0';
            out_buff[1] = ' ';
            //converting total cost to string
            char temp[MAX_LINE];
            if(tot_cost[j] == 0){
                temp[0] = '0';
                temp[1] = '\0';
            }
            else itoa(tot_cost[j], temp);
            //now putting this value in out_buff
            int i = 2;
            while(temp[i-2]!= '\0'){
                out_buff[i] = temp[i-2];
                i++; 
            }
            out_buff[i] = '\0';
            //sending this data to client
            write(clientSocket, out_buff, sizeof(out_buff));
            close(clientSocket);
            break;
        }
        //user is requesting for an item
        else if(reqType == '0'){
            char upc[MAX_LINE], num_of_items[MAX_LINE];
            int i = 2;
            int k = 0;
            //storing the upc code and number of items
            while(in_buff[i] != '\0' && in_buff[i]!= '\n' && in_buff[i] != ' '){
                upc[k++] = in_buff[i++];
            }
            i++;
            upc[k] = '\0';
            k =0;
            int x = i;
            

            //error in upc code
            if(upc == NULL || strlen(upc) != 3){
                int i=0;
                printf("%ld\n", sizeof(upc));
                // printf("%s\n", upc);
                char temp[] = "1 Protocoll Error";
                while(temp[i] != '\0'){
                    out_buff[i] = temp[i];
                    i++;
                }
                out_buff[i] = '\0';
                write(clientSocket, out_buff, sizeof(out_buff));

                printf("Give the correct UPC code\n");
            }
            else{
                int cost = -1;
                char * desc;
                //reading data from the database (txt file)
                FILE * database;
                database = fopen("database.txt" , "r");	
                if(database == NULL){
                    cost = -1;
                    desc = "Error opening in the database";
                }
                else{
                    char tempupc[100];
                    char tmpdesc[100];
                    int dd;
                    // Reading the file and comparing UPC code with each line of file to find the correct entry.
                    while(fscanf(database , "%s %d %s\n" , tempupc , &dd , tmpdesc) != EOF){
                    // Comparing strings.		
                        if(!strncmp(tempupc,upc,3)){
                            desc = (char *) malloc(sizeof(tmpdesc));
                            strcpy(desc , tmpdesc);
                            cost = dd;
                            break;
                        }
                    }
                    fclose(database);
                    //item not found in the database
                    if(cost == -1){
                        int i=0;
                        char temp[] = "1 UPC code not found in the database";
                        while(temp[i] != '\0'){
                            out_buff[i] = temp[i];
                            i++;
                        }
                        out_buff[i] = '\0';
                        write(clientSocket, out_buff, sizeof(out_buff));
                        
                    }
                    //item found in the database
                    else{
                        while(in_buff[x] != '\0' && in_buff[x]!= '\n' && in_buff[x] != ' '){
                            num_of_items[k++] = in_buff[x++];
                        }
                        num_of_items[k] = '\0';
                        // adding cost*num_of_items to total cost
                        int num = atoi(num_of_items);
                        printf("%s\n", num_of_items);
                        printf("%d\n", num);
                        tot_cost[j] += cost*num;
                        out_buff[0] = '0';
                        out_buff[1] = ' ';
                        int k=2;

                        // the cost along with the description will be sent to the client
                        char temp[MAX_LINE];
                        if(!cost){
                            temp[0] = '0';
                            temp[1] = '\0';
                        }
                        else itoa(cost,temp);
                        
                        int ctr=0;
                        
                        while(temp!=NULL && temp[ctr]!='\0'){
                            out_buff[k++] = temp[ctr++];
                        }
                        out_buff[k++] = ' ';
                        ctr=0;
                        
                        while(desc != NULL && desc[ctr]!='\0'){
                            out_buff [k++] = desc[ctr++];
                        }
                        out_buff[k]='\0';
                        printf("Printing upc code, num_of_items, description, cost ans total cost respectively\n");
                        printf("%s %s %s %d %d\n",upc , num_of_items , desc, cost, tot_cost[j]);
                        //sending the response to client
                        write(clientSocket , out_buff , sizeof(out_buff) );
                    }
                }
            }

        }
        //user has given wrong request type
        else{
            int i=0;
            char temp[] = "1 Protocol Error";
            while(temp[i] != '\0'){
                out_buff[i] = temp[i];
                i++;
            }
            out_buff[i] = '\0';
            write(clientSocket, out_buff, sizeof(out_buff));
            printf("Give the correct value of request type\n");
        }
    }
}

int main(int argc, char ** argv){
    funn(1);
    if(argc!=2){
        printf("Enter correct number of arguments\n");
        exit(0);
    }
    //setting all the clients as -1, which means there is no client and alsp the total cost as 0
    for(int i=0; i<MAX_CLIENTS; i++)clients[i] = -1;
    //setting the signal handler (when the user presses CTRL + C)
    signal(SIGINT, ctrlchandler);
    //initialising the socket
    //listenSocket is used by the client to send connection request and clientSocket to estaablish connection 
    //through which messages will be transferred
	int listenSocket, clientSocket;
    struct sockaddr_in serverA;
    struct sockaddr_in serverAdd;
    struct sockaddr_in serverAddress;
	struct sockaddr_in clientAddress;
	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(listenSocket<0){
        printf("Error in creating listening socket\n");
        exit(0);
    }
    serverInit(serverA);
    serverInit(serverA);
	printf("Listening Socket created successfully\n");
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons((int)atoi(argv[1]));
    serverInit(serverAdd);
    serverInit(serverAdd);
    //binding the listenSocket to serverAddress
    if(bind(listenSocket,(struct sockaddr*)&serverAddress,sizeof(serverAddress))!=0){
        printf("Error in binding\n");
		exit(0);
    }
    

    //checking if the number of clients don't exceed the MAX_CLIENTS
    if(listen(listenSocket,MAX_CLIENTS)<0){
		printf("Error in listening\n");
		exit(0);
	}

    
    //infinite loop
    while(1){
        //needed while allocating socket
        socklen_t clientLength = sizeof(clientAddress);
        //if the connection request is accepted then clientSocket will be it's socket number
        clientSocket = accept(listenSocket,(struct sockaddr*)&clientAddress, &clientLength);
		//handling error
		if(clientSocket<0) {
			printf("Error in creating socket connection\n");
			exit(0);
		}
        pid_t clientPid = fork();
        if(clientPid <0){
            printf("error in fork\n");
            serverInit(serverA);
            serverInit(serverAdd);
            exit(0);
        }
        // If childPid == 0 then the server must stop recieving any connection requests for this child process
        if(!clientPid){
            if(close(listenSocket<0)){
                printf("Error in closing listeing socket\n");
                serverInit(serverA);
                serverInit(serverAdd);
                exit(0);
            }
            //function to habdle client's requests
			func(clientSocket);
            serverInit(serverA);
            serverInit(serverAdd);
			exit(0);
        }
        if(close(clientSocket)<0){
            printf("Error in closing the socket\n");
            exit(0);
        }
    }

}