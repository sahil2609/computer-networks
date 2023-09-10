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

// function that will be called to take input and send it to the server

void func(int clientSocket){
    char se_buff[MAX_LINE]; // buffer to take input
    char rcv_buff[MAX_LINE]; //buffer to receive input from server
    while(1){ //while the user is giving input
        memset(se_buff,0,MAX_LINE); //initialising the buffer with all 0's
        memset(rcv_buff,0,MAX_LINE);
        printf("Enter the request in the format, Request_ Type || UPC-Code || Number\n");


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
            printf("Client Exit\n");
            break;
        }
        //rcv_buff[0] == '2' => server exits with ctrl-c and hence sends the message to client to close connection
        if(rcv_buff[0] == '2'){
            printf("Server down, Client exiting\n");
            break;
        }

    }
}

int main(int argc, char ** argv){
	struct sockaddr_in serverAddress;
    int clientSocket;
    //if the number of input arguments is not equal to 3

    if(argc != 3){
        printf("Number of input argumetns should be equal to 3\n");
        exit(0);
    }

	clientSocket = socket(AF_INET,SOCK_STREAM,0);
	
	//negative client sokcet indicates error
    
	if(clientSocket<0){
		printf("Error in creating client socket\n");
		exit(0);
	}

    //success message
	printf("Socket created successfully\n");
	
	// initializes the serverAddress with all 0's
    memset(&serverAddress,0,sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons( (int)atoi( argv[2] ) );
	if(inet_pton(AF_INET, argv[1], &serverAddress.sin_addr) <= 0){
		printf("INET PTON error!! \n");
		exit(0);
	}



    //if there is error in the 3-way handshake performed by TCP
	if (connect( clientSocket , (struct sockaddr *) &serverAddress , sizeof(serverAddress)) < 0){
		printf("Error\n");
		exit(0);
	}
	
	printf("TCP Connection established\n\n");

	// function to handle the incoming requests
	func(clientSocket);	
	exit(0);

	return 0;

}