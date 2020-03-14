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
* Helper method that just reads in a message and echos the message.  I made an echo server
* at first then extended it into the final code, so this method might not be used in the
* final submission. 
**/
void TCPServer::readEcho(){
    zeroBuf();
    valread = read (new_sock, buf, 1024);
    std::cout << buf << std::endl;
    buf[valread] = '\0';
    sockSend(new_sock, &buf[0]);
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
        //sockSend(this->new_sock, initMessage.c_str()); // Send the welcome message
    }
}


/**
* Helper function that will handle data from an existing client
**/
void TCPServer::handleExistingClient(int i){
    zeroBuf();
    if( (this->valread = read (i, this->buf, 1024)) <= 0){
        // recieved an error, or connection was closed by client
        if (this->valread == 0){
            // Client closed connection
            std::cout << "The client on socket: " << i << " closed the connection." << std::endl; 
        } else {
            perror("Failure in read() in TCPServer.\n");
        }
        close(i); // Make sure we close out the socket
        FD_CLR(i, &(this->master)); // Remove the client socket from the master FD list
    } else {
        // Handle the data from the client
        //parseData(i);
        // <TODO> change this 

        // This is how you send something to socket i
        const char* hello = "1000";
        sockSend(i, hello);
    }
}


/**
* Helper function that will parse passed in data from a client and send data back to the client
* 
* Input: the FD that we are parsing the input for, which is used to send the correct message back
**/
void TCPServer::parseData(int i){
    // buf has the data that was passed in from the client, so check it for our menu items:
    if(strcmp(buf, "hello") == 0){
        const char* hello = "Welcome to Capt White's TCPserver for Distributed Systems!";
        sockSend(i, hello); 
        std::cout << "User on socket " << i << " sent hello command" << std::endl;
    } else if(strcmp(buf, "1") == 0){
        const char* one = "C++ got the OOP features from Simula67 Programming language.";
        sockSend(i, one); 
        std::cout << "User on socket " << i << " sent 1 command" << std::endl;
    } else if(strcmp(buf, "2") == 0){
        const char* two = "Not purely object oriented: We can write C++ code without using classes and it will compile without showing any error message.";
        sockSend(i, two); 
        std::cout << "User on socket " << i << " sent 2 command" << std::endl;
    } else if(strcmp(buf, "3") == 0){
        const char* three = "C and C++ invented at same place i.e. at T bell laboratories.";
        sockSend(i, three); 
        std::cout << "User on socket " << i << " sent 3 command" << std::endl;
    } else if(strcmp(buf, "4") == 0){
        const char* four = "Concept of reference variables: operator overloading borrowed from Algol 68 programming language.";
        sockSend(i, four); 
        std::cout << "User on socket " << i << " sent 4 command" << std::endl;
    } else if(strcmp(buf, "5") == 0){
        const char* five = "A function is the minimum requirement for a C++ program to run.";
        sockSend(i, five); 
        std::cout << "User on socket " << i << " sent 5 command" << std::endl;
    } else if(strcmp(buf, "passwd") == 0){
        const char* passwd = "This feature is not currently implemented.";
        sockSend(i, passwd); 
        std::cout << "User on socket " << i << " sent passwd command" << std::endl;
    } else if(strcmp(buf, "exit") == 0){
        const char* exitCmd = "Have a great day! ";
        sockSend(i, exitCmd); 
        close(i); // Make sure we close out the socket
        FD_CLR(i, &(this->master)); // Remove the client socket from the master FD list
        std::cout << "The client on socket: " << i << " closed the connection." << std::endl; 
    } else if(strcmp(buf, "menu") == 0){
        if(secretMenu){
            sockSend(i, this->secretMenuText.c_str()); 
        } else {
            sockSend(i, this->commands.c_str()); 
        }
    } else if(strcmp(buf, "uuddlrlrab") == 0){
        const char* konami = "You cracked the code!";
        sockSend(i, konami); 
        std::cout << "The client on socket: " << i << " cracked the code, we must adapt..." << std::endl; 
        this->secretMenu = true; 
    } else {
        sockSend(i, this->incorrect.c_str()); 
        std::cout << "User on socket " << i << " sent an incorrect command" << std::endl;
    }
    
}


//============================================================================================================================================================================
/** 
* Given methods to implement below: 
**/


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
                    handleNewClient();
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
