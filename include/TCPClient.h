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

#include <string.h>
#include "Client.h"

// The amount to read in before we send a packet
const unsigned int stdin_bufsize = 50;
const unsigned int socket_bufsize = 100;

class TCPClient : public Client
{
public:
   TCPClient();
   ~TCPClient();

   virtual void connectTo(const char *ip_addr, unsigned short port);
   virtual void handleConnection();

   virtual void closeConn();

private:
   int client_sock = 0; 
   int valread = 0; 
   int opt = 1; 
   struct sockaddr_in address; 
   char buf[1024] = {0}; // Initialize the buffer to 0 

   

};


#endif
