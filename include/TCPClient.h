#pragma once

#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <list>
#include <boost/multiprecision/cpp_int.hpp>

#include <string.h>
#include "Client.h"

// Set up the namespace for the boost variables
namespace mp = boost::multiprecision;

// The amount to read in before we send a packet
const unsigned int stdin_bufsize = 50;
const unsigned int socket_bufsize = 100;

/* "UNSIGNED int type to hold original value and calculations" */
#define LARGEINT mp::uint128_t

/* "UNSIGNED int twice as large as LARGEINT (bit-wise)" */
#define LARGEINT2X mp::uint256_t

/* "SIGNED int made of twice the bits as LARGEINT2X" */
#define LARGESIGNED2X mp::int512_t

class TCPClient : public Client
{
public:
   TCPClient();
   ~TCPClient();

   virtual void connectTo(const char *ip_addr, unsigned short port);
   virtual void handleConnection();

   virtual void closeConn();

protected:
   LARGEINT calcPollardsRho(LARGEINT n);
   LARGEINT2X modularPow(LARGEINT2X base, int exponent, LARGEINT2X modulus);


private:
   int client_sock = 0; 
   int valread = 0; 
   int opt = 1; 
   struct sockaddr_in address; 
   char buf[1024] = {0}; // Initialize the buffer to 0 

};


#endif
