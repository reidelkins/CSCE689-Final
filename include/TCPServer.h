#pragma once

#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <sys/select.h>

#include <list>
#include <boost/multiprecision/cpp_int.hpp>


#include "Server.h"

// Set up the namespace for the boost variables
namespace mp = boost::multiprecision;

/* "UNSIGNED int type to hold original value and calculations" */
#define LARGEINT mp::uint128_t

/* "UNSIGNED int twice as large as LARGEINT (bit-wise)" */
#define LARGEINT2X mp::uint256_t

/* "SIGNED int made of twice the bits as LARGEINT2X" */
#define LARGESIGNED2X mp::int512_t

class TCPServer : public Server 
{
public:
   TCPServer();
   ~TCPServer();

   // Helper methods for setting up and binding the main server socket:
   void sockSetup(); 
   void sockOptions();
   void sockBind(const char *ip_addr, short unsigned int port);

   // Helper method to send messages on a connected server socket
   void sockSend(int sock, const char* msg);

   // Helper method that just reads in a message and echos the message, probably won't
   // be used in final submission, but was a stepping stone
   void readEcho();

   // Helper method that checks to see if the passed in buffer says quit
   int quitCheck(const char *buffer);

   // Helper method to clear out buf, set everything to '\0'
   void zeroBuf();

   // Helper method for listen call
   void serverListen();

   // Helper method for select call, puts return value of the select() call into this->select_result
   void serverSelect();

   // Helper function that will handle accepting a new client to our server
   void handleNewClient();
   
   // Helper function that will handle data from an existing client
   void handleExistingClient(int i);

   // Helper function that will parse passed in data from a client and send data back to the client
   void parseData(int i);

   // Given methods to implement:
   void bindSvr(const char *ip_addr, unsigned short port);
   void listenSvr();
   void shutdown();

   void factor();
   bool isPrimeBF(LARGEINT n, LARGEINT &divisor);
  
   LARGEINT getCurrentValue() {return current_value;}
   //changes value for what is set to be factored next
   void changeValue(LARGEINT n) {current_value = n;}

   void printPrimes();
   //sends the socket connected to the current client
   //where work needs to be sent
   void sendNum(int i);
   
protected:
   void factor(LARGEINT n);

private:
   // Note, I choose to initialize some variables here instead of the constructor, which I'm able to do as of c++11.
   // The difference between doing it here vs in the constructor is when I init something here it happens at compile time,
   // and I'm okay with that happening at compile time because it is such a small amount of code I'm compiling I would 
   // rather keep my constructors clear of initializing variables when possible.  I think on actual production code of a 
   // server you would also want to minimize startup time in this way, but it depends on your use case. 
   int server_sock = 0; 
   int opt = 1; 
   struct sockaddr_in address; 
   char buf[1024] = {0}; // Used to read in client messages, zero it out here
   int valread = 0; // Used to check any read() calls

   // What we are trying to factor oringally
   LARGEINT original_value;

   //current value that is being factored
   LARGEINT current_value;

   // The list of all the prime factors of origional_value
   std::list<LARGEINT> primes;

   //numbers sent back from client that still need to be factored
   std::list<LARGEINT> divisors;

   // If factor has been called on a number x times, check to see if the number is prime
   const unsigned int primecheck_depth = 10;
   int iters = 0;

   //check if prime factorization is finished
   bool finished = false;
    
   fd_set master; // Master FD list
   fd_set read_fds; // list used for select()

   int biggestFD = 0; // int used to keep track of the biggest FD yet for the select() call
   int new_sock = 0; // Used when there is a new socket
   int select_result = 0; // Used to check output of select()
   int addrlen = 0; // Used to call accept

};


#endif
