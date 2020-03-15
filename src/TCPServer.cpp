#include "TCPServer.h"

/**
 * I am using the following sites as references to create this file and the rest of my code base: 
 * https://www.geeksforgeeks.org/socket-programming-cc/
 * https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/
 * https://www.geeksforgeeks.org/tcp-and-udp-server-using-select/
 * https://stackoverflow.com/questions/15673846/how-to-give-to-a-client-specific-ip-address-in-c
 * http://beej.us/guide/bgnet/html/  // This document is amazing. 
 *
 * Author: Joshua H. White
**/

/**
* Default Constructor. Also calls the constructor for Server. 
**/
TCPServer::TCPServer() : Server() {
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    LARGEINT find("463463374607431768211451");
    
    original_value = find;
    current_value = original_value;
    factor();
}

/**
* Destructor, does nothing for now. 
**/
TCPServer::~TCPServer() {}


/**
* Helper methods for the server:
**/

/**
* Helper method to set up the socket for the server. 
**/ 
void TCPServer::sockSetup(){
    this->server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if( this->server_sock == 0 ){
        perror("Failure in socket() in TCPServer.\n");
        exit(EXIT_FAILURE);
    }

    // Setup biggest FD here, because server_sock is the only FD we currently have
    this->biggestFD = this->server_sock; 
}


/**
* Helper method that wraps the setsockopt call. 
*
* Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
**/
void TCPServer::sockOptions(){
    if ( setsockopt(this->server_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &this->opt, sizeof(this->opt)) ){
        perror("Failure in setsockopt() in TCPServer.\n");
        exit(EXIT_FAILURE);
    }
}


/**
* Helper method that wraps the bind call, and any required setup. 
*
* Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
**/
void TCPServer::sockBind(const char *ip_addr, short unsigned int port){
    // Set up the sockaddr_in struct here:
    this->address.sin_family = AF_INET; 
    // this->address.sin_addr.s_addr = INADDR_ANY; // Won't use this, this uses the computer's default IP of 127.0.0.1
    inet_pton(AF_INET, ip_addr, &(this->address.sin_addr)); // Inserts the IP address as a string into the sockaddr_in struct
    this->address.sin_port = htons( port ); // htons is shorthand for Host-to-Network short
    
    // Now bind the socket, if it fails print an error message and exit
    if ( ( bind(this->server_sock, (struct sockaddr *)&this->address, sizeof(this->address)) ) < 0 ){
        perror("Failure in bind() in TCPServer.\n");
        exit(EXIT_FAILURE);
    }
}


/**
* Helper method to send messages on a connected server socket
**/
   void TCPServer::sockSend(int sock, const char* msg){
       send(sock, msg, strlen(msg), 0);
}


/**
* Helper method that checks to see if the passed in buffer says quit
*
* Returns: 0 if the parameter is "quit", -1 otherwise
**/
int TCPServer::quitCheck(const char *buffer){
    std::string q = "quit";
    if(q.compare(buffer) == 0){
        return 0; 
    }
    return -1; 
}


/**
* Helper method to clear out buf, set everything to '\0'
**/
void TCPServer::zeroBuf(){
    for(int i = 0; i < 1024; i++){
        this->buf[i] = '\0';
    }
}


/**
* Helper method for listen call 
**/
void TCPServer::serverListen(){ 
    if( listen(this->server_sock, 20) < 0){
        perror("Failure in listen() in TCPServer.\n");
        exit(EXIT_FAILURE);
    }
    // Make sure to add the server socket to the master FD set
    FD_SET(this->server_sock, &(this->master)); 
}


/**
* Helper method for select call, puts return value of the select() call into this->select_result
**/
void TCPServer::serverSelect(){
    this->select_result = select( this->biggestFD + 1, &(this->read_fds), NULL, NULL, NULL);
    if(this->select_result == -1){
        perror("Failure in listen() in TCPServer.\n");
        exit(EXIT_FAILURE);
    }
}


/**
* Helper function that will handle accepting a new client to our server
*
* <TODO> Add some sort of connection object
**/
void TCPServer::handleNewClient(){
    this->addrlen = sizeof(this->address); // set up addrlen for the accept call
    this->new_sock = accept(this->server_sock, (struct sockaddr *)&this->address, (socklen_t*)&addrlen);
    // Error check
    if ( new_sock < 0){
        perror("Failure in accept() in TCPServer.\n");
        exit(EXIT_FAILURE);
    } else {
        // Need to add the new FD to the master FD set
        FD_SET(this->new_sock, &(this->master)); 
        // Keep track of largest FD
        if (this->new_sock > this->biggestFD){
            this->biggestFD = this->new_sock;
        }
        std::cout << "New connection on socket " << this->new_sock << std::endl; // Notify the server of the new connection
        //sockSend(this->new_sock, getCurrentValue().str());
        //sockSend(this->new_sock, initMessage.c_str()); // Send the welcome message
    }
}




/**
* Helper function that will handle data from an existing client
**/
void TCPServer::handleExistingClient(int i){
    zeroBuf();
    // if( (this->valread = read (i, this->buf, 1024)) <= 0){
    //     // recieved an error, or connection was closed by client
    //     if (this->valread == 0){
    //         // Client closed connection
    //         std::cout << "The client on socket: " << i << " closed the connection." << std::endl; 
    //     } else {
    //         perror("Failure in read() in TCPServer.\n");
    //     }
    //     close(i); // Make sure we close out the socket
    //     FD_CLR(i, &(this->master)); // Remove the client socket from the master FD list
    // } else {
    //     // Handle the data from the client
    //     //parseData(i);
    //     // <TODO> change this 

    //     // This is how you send something to socket i
    //     const char* hello = "1000";
    //     sockSend(i, hello);
    // }

}


/**********************************************************************************************
 * bindSvr - Creates a network socket and sets it nonblocking so we can loop through looking for
 *           data. Then binds it to the ip address and port
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
**********************************************************************************************/
void TCPServer::bindSvr(const char *ip_addr, short unsigned int port) {
    // Socket setup:
    sockSetup();

    // Set up the options of the socket: 
   sockOptions();

    // Bind the socket:
    sockBind(ip_addr, port);
}


/**********************************************************************************************
 * listenSvr - Performs a loop to look for connections and create TCPConn objects to handle
 *             them. Also loops through the list of connections and handles data received and
 *             sending of data.   
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
**********************************************************************************************/
void TCPServer::listenSvr() {
    serverListen();  // Listen helper

    // Now is the main while loop
    while(true) {
        // copy over master into read_fds
        this->read_fds = this->master;
        zeroBuf();
        //FD_ZERO(&(this->read_fds);
        serverSelect();
        // Now loop through all connections and check each FD for data to read from a connected client
        // or if we need to add a new client
        for(int i = 0; i <= this->biggestFD; i++){
            // Check to see if i is the FD accept() hit on
            if(FD_ISSET(i, &read_fds)){ 
                if(i == this->server_sock){ 
                    // If i is our server_sock then we have a new connection
                    std::cout << "before handle new client\n";
                    handleNewClient();
                    std::cout << "after handle new client\n";
                } else { 
                    // Else a client sent us something
                    handleExistingClient(i);
                }
            }
        }

        // <TODO> add logic here to do pollards rho 

        // logic
        // send string
        // read string then do something with string
    } 
}

/**********************************************************************************************
 * shutdown - Cleanly closes the socket FD.
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::shutdown() {
    if( close(server_sock) ){
        perror("Failure in close() in TCPServer.");
        exit(EXIT_FAILURE);
    }
}

/**
* factor - Calculates a single prime of the given number and recursively calls
*          itself to continue calculating primes of the remaining number. Variation
*          on the algorithm by Yash Varyani on GeeksForGeeks. Uses a single
*          process 
**/
void TCPServer::factor(){

   // First, take care of the '2' factors
   LARGEINT newval = getCurrentValue();
   
   while (newval % 2 == 0) {
      primes.push_back(2);

      std::cout << "Prime Found: 2\n";

      newval = newval / 2;
   } 

   // Now the 3s
   while (newval % 3 == 0) {
      primes.push_back(3);
      std::cout << "Prime Found: 3\n";
      newval = newval / 3;
   }
   changeValue(newval);

   // Now use Pollards Rho to figure out the rest. As it's stochastic, we don't know
   // how long it will take to find an answer. Should return the final two primes
   //factor(newval); 
}

/**
* factor - same as above function, but can be iteratively called as numbers are
*          discovered as the number n can be passed in
**/
void TCPServer::factor(LARGEINT n) {
    // already prime
    if (n == 1) {
        return;
    }

    std::cout << "Factoring: " << n << std::endl;

    bool div_found = false;
    unsigned int iters = 0;

    while (!div_found) {
      
        std::cout << "Starting iteration: " << iters << std::endl;

        // If we have tried Pollards Rho a specified number of times, run the
        // costly prime check to see if this is a prime number. Also, increment
        // iters after the check
        if (iters++ == primecheck_depth) {
            std::cout << "Pollards rho timed out, checking if the following is prime: " << n << std::endl;

            LARGEINT divisor;
            if (isPrimeBF(n, divisor)) {
                std::cout << "Prime found: " << n << std::endl;

                primes.push_back(n);
                return;
            } else {   // We found a prime divisor, save it and keep finding primes
                std::cout << "Prime found: " << divisor << std::endl;

                primes.push_back(divisor);
                return factor(n / divisor);
            }				
        }

        // We try to get a divisor using Pollards Rho
        //LARGEINT d = calcPollardsRho(n);
        LARGEINT d = 0;
        //CHANGE

        if (d != n) {
            std::cout << "Divisor found: " << d << std::endl;

            // Factor the divisor
            factor(d);

            // Now the remaining number
            factor((LARGEINT) (n/d));
            return;
        }

        // If d == n, then we re-randomize and continue the search up to the prime check depth
    }
    throw std::runtime_error("Reached end of function--this should not have happened.");
    return;
}

/*******************************************************************************
* Uses the Primality Test algorithm with the 6k optimization, can be found:
*    https://en.wikipedia.org/wiki/Primality_test
*             
*
* Params:  n - the number to test for a prime 
*   divisor -return value of the discovered divisor if not prime
*
* Returns: true if prime, false otherwise
*******************************************************************************/
bool TCPServer::isPrimeBF(LARGEINT n, LARGEINT &divisor) {
    //std::cout << "Checking if prime: " << n << std::endl;
    divisor = 0;

    // Take care of simple cases
    if (n <= 3) {
        // divisor = n; // Might need to do this? check where its called <TODO>
        return n > 1;
    }
    else if ((n % 2) == 0) {
        divisor = 2;
        return false;
    } else if ((n & 3) == 0) {
        divisor = 3;
        return false;
    }

    // Every prime number greater than 6 is a multiple of 6k+1 or 6k-1 where k is 
    // a prime number.  This for loop below handles this logic for us.  The key thing
    // we need to watch out for is an overflow of the datatype when we do the i*i part,
    // so since we are using a 64 bit int we will use 128 bit ints to hold the i*i math
    // incase we bust that 64 int size (causing overflow errors). When we make this
    // class handle 128 bit ints this will have to be 256, and 512 for 256 bit numbers.
    LARGEINT2X k = n;
    for (LARGEINT2X i=5; i * i < k; i = i+6) {
        if ((k % i == 0) || (k % (i+2) == 0)) {
            divisor = (LARGEINT)i; // Downcast it to a LARGEINT to put it in divisor
            return false;
        }
    }
    return true;
}

/**
 * Print all of the numbers in the prime list to the terminal
 **/
void TCPServer::printPrimes(){
    std::cout << "The list of prime factors for the input number: " << std::endl;
    std::cout << original_value << std::endl; 

    for(auto const& p : primes){
        std::cout << p << ", "; 
    }
    std::cout << std::endl; 
    
    
}