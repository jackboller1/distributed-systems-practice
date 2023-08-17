
Compile the code using the provided makefile:

    make

To clear the directory (and remove .txt files):
   
    make clean

To run the server on port 3010:

    ./tsd -p 3010

To run the client  

    ./tsc -h host_addr -p 3010 -u user1

To run the coordinator

    ./coordinator -p <portNum>

To run the synchronizer

    ./synchronizer -cip <coordinatorIP> -cp <coordinatorPort> -p <portNum> -id <synchronizerId>

To run the client

    ./client -cip <coordinatorIP> -cp <coordinatorPort> -id <clientId>

To run the server:

    ./server -cip <coordinatorIP> -cp <coordinatorPort> -p <portNum> -id <idNum> -t <master/slave>

