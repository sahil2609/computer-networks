//Server Side Programming
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>


// Max out_buff side of the client
#define MAX_LINE 1024
#define MAX_CLIENTS 100 // Max number of clients allowed for the server.
int clients[MAX_CLIENTS]; //array to store clients
int tot_cost[MAX_CLIENTS]; //array to store total cost for each client


// signal handler to control the action of server down by pressing CTRL + C
void ctrlCHandler( int num )
{
	char out_buff[MAX_LINE];
	signal(SIGINT, ctrlCHandler);
	for(int i=MAX_CLIENTS-1;i>=0;i--){

		if(clients[i] != -1){
			// Sending message to client
			memset(out_buff,0,MAX_LINE);	
			char *temp = "2 Server Down";
			int k = 0;
			for(int i = 0; temp[i]; i++){
				out_buff[k++] = temp[i];
			}
			out_buff[k] = '\0';
			write(clients[i] , out_buff , sizeof(out_buff));

			if(close(clients[i]) < 0){
				printf("Socket Number %d could not be released\n",clients[i]);
			}
			clients[i] = -1;	
		}
	}
	printf("Server Exited Gracefully\n" );
	exit(0);
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
    if(f ==0){
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
            else itoa(tot_cost[j], temp, 10);
            //now putting this value in out_buff
            int i = 2;
            while(temp[i-2]!= '\0'){
                out_buff[i] = temp[i-2];
                i++; 
            }
            out_buff[i] = '\0';
            tot_cost[j] = 0;
            //sending this data to client
            write(clientSocket, out_buff, sizeof(out_buff));
            close(clientSocket);
            break;
        }
        //user is requesting for an item
        else if(reqType == '0'){
            char upc[MAX_LINE], num_of_items[MAX_LINE];
            int i =2;
            int k = 0;
            //storing the upc code and number of items
            while(in_buff[i] != '\0' && in_buff[i]!= '\n' && in_buff[i] != ' '){
                upc[k++] = in_buff[i++];
            }
            upc[k] = '\0';
            k =0;
            while(in_buff[i] != '\0' && in_buff[i]!= '\n' && in_buff[i] != ' '){
                num_of_items[k++] = num_of_items[i++];
            }
            num_of_items[k] = '\0';

            //error in upc code
            if(upc == NULL || sizeof(upc) != 3){
                int i=0;
                char temp[] = "1 Protocol Error";
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
                    char dupc[100];
                    char ddesc[100];
                    int dd;
                    // Reading the file and comparing UPC code with each line of file to find the correct entry.
                    while(fscanf(database , "%s %d %s\n" , dupc , &dd , ddesc) != EOF){
                    // Comparing strings.		
                        if(strncmp(dupc , upc , 3) == 0){
                            desc = (char *) malloc(sizeof(ddesc));
                            strcpy(desc , ddesc);
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
                        // adding cost*num_of_items to total cost
                        int num = atoi(num_of_items);
                        tot_cost[j] += cost*num;
                        out_buff[0] = '0';
                        out_buff[1] = ' ';
                        int k=2;

                        // the cost along with the description will be sent to the client
                        char temp[MAX_LINE];
                        if(cost == 0){
                            temp[0] = '0';
                            temp[1] = '\0';
                        }
                        else itoa( cost , temp,10 );
                        
                        int ctr=0;
                        
                        while( temp!=NULL && temp[ctr]!='\0' )
                        {
                            out_buff[k++] = temp[ctr++];
                        }
                        out_buff[k++] = ' ';
                        ctr=0;
                        
                        while( desc != NULL && desc[ctr]!='\0' )
                        {
                            out_buff [k++] = desc[ctr++];
                        }
                        out_buff[k]='\0';
                        printf("Printing upc code, num_of_items, description, cost ans total cost respectively\n");
                        printf("%s %s %s %d %d\n",upc , num , desc, cost, tot_cost[j]);
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

}