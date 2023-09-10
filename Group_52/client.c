// client side programming
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

// Max buffer side of the client
#define MAX_LINE 1024

//function to initialise the server
void serverInit(struct sockaddr_in serverAddress){
    // initializes the serverAddress with all 0's
    memset(&serverAddress,0,sizeof(serverAddress));
}

// function that will be called to take input and send it to the server

void func(int clientSocket){
    char se_buff[MAX_LINE]; // buffer to take input
    char rcv_buff[MAX_LINE]; //buffer to receive input from server
    while(1){ //while the user is giving input
        memset(se_buff,0,MAX_LINE); //initialising the buffer with all 0's
        memset(rcv_buff,0,MAX_LINE);
        printf("Directions from client side: Enter the request in the format, Request_ Type || UPC-Code || Number\n");


        // taking the input
        fgets(se_buff,MAX_LINE,stdin);
        //now sending this request to server
        write(clientSocket, se_buff, sizeof(se_buff)); 
        // reading the data from server
        read(clientSocket, rcv_buff, sizeof(rcv_buff));


        printf("Below is the response from server\n");
        printf("%s\n", rcv_buff);

        //se_buff[0] == '1' => client wants to close the connection
        if(se_buff[0] == '1'){
            printf("Directions from client side: Client Exit\n");
            break;
        }
        //rcv_buff[0] == '2' => server exits with ctrl-c and hence sends the message to client to close connection
        if(rcv_buff[0] == '2'){
            printf("Directions from client side: Server down, Client exiting\n");
            break;
        }

    }
}

void funn(int f){
    if(f){
        printf("Server side programming\n");
    }
    else{
        printf("Client side Programming\n");
    }
}

int main(int argc, char ** argv){
	
    //if the number of input arguments is not equal to 3
    funn(0);
    if(argc != 3){
        printf("Directions from client side: Number of input argumetns should be equal to 3\n");
        exit(0);
    }
    
    int clientSocket;

	clientSocket = socket(AF_INET,SOCK_STREAM,0);
	
	//negative client sokcet indicates error
    
	if(clientSocket<0){
		printf("Directions from client side: Error in creating client socket\n");
		exit(0);
	}
    struct sockaddr_in serverA;
    struct sockaddr_in serverAdd;
    struct sockaddr_in serverAddress;

    //success message
	printf("Directions from client side: Socket created successfully\n");

    serverInit(serverA);
	// initializes the serverAddress with all 0's
    memset(&serverAddress,0,sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons((int)atoi(argv[2]) );
	if(inet_pton(AF_INET, argv[1], &serverAddress.sin_addr) <= 0){
		printf("Directions from client side: INET PTON error!! \n");
		exit(0);
	}
    serverInit(serverAdd);
    serverInit(serverA);


    //if there is error in the 3-way handshake performed by TCP
	if (connect(clientSocket , (struct sockaddr *)&serverAddress,sizeof(serverAddress))<0){
		printf("Directions from client side: Error\n");
        serverInit(serverAdd);
		exit(0);
	}
	
	printf("Directions from client side: TCP Connection established\n\n");
    serverInit(serverAdd);
    serverInit(serverA);
	// function to handle the incoming requests
	func(clientSocket);	
	exit(0);

	return 0;

}