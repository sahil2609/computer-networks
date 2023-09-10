// client side programming
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

}