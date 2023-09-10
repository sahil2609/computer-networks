#Execute as follows:-
gcc -o server server.c 
gcc -o client client.c

#Run the server
./server  port_number

#open new terminal which is in the same directory as executible present
./client  input_ip_address  port_number

#give input to client 
#request type=0 upc code number of items
0 101 3
#close
1