#include "TCPClient.h"

#include <iostream>
#include <list>
#include <string>
#include <algorithm>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/integer/common_factor.hpp>


//This note is from Capt. Joshua H. White's HW1. Our group used this HW as a base for the client/server used in this assignment
/**
 * I am using the following sites as references to create this file: 
 * https://www.geeksforgeeks.org/socket-programming-cc/
 * https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/
 * https://www.geeksforgeeks.org/tcp-and-udp-server-using-select/
 * https://stackoverflow.com/questions/15673846/how-to-give-to-a-client-specific-ip-address-in-c
 * http://beej.us/guide/bgnet/html/  // This document is amazing. 
 * 
 * NOTE: I am also not going to break out the client code into helper methods as much as I did the server code 
 * because the code is much more simple and I don't think it would increase readability.  
 *
 * Author: Joshua H. White
**/

// Set up the namespace for the boost variables
namespace mp = boost::multiprecision;

/**********************************************************************************************
 * TCPClient (constructor) - Creates a Stdin file descriptor to simplify handling of user input. 
 *
**********************************************************************************************/
TCPClient::TCPClient() : Client(){}

/**********************************************************************************************
 * TCPClient (destructor) - No cleanup right now
 *
**********************************************************************************************/
TCPClient::~TCPClient() {}

/**********************************************************************************************
 * connectTo - Opens a File Descriptor socket to the IP address and port given in the
 *             parameters using a TCP connection.
 *
 *    Throws: socket_error exception if failed. socket_error is a child class of runtime_error
**********************************************************************************************/
void TCPClient::connectTo(const char *ip_addr, unsigned short port) {
    // Create the socket for the client
    this->client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if( this->client_sock == 0 ){
        perror("Failure in socket() in TCPClient.\n");
        exit(EXIT_FAILURE);
    }

    this->address.sin_family = AF_INET; 
    this->address.sin_port = htons( port ); // htons is shorthand for Host-to-Network short

    // Convert the IP address to binary form
    if(inet_pton(AF_INET, ip_addr, &address.sin_addr) <= 0){
        perror("Invalid address in TCPClient.\n");
        exit(EXIT_FAILURE);
    }

    // Now try to connect
    if(connect(this->client_sock, (struct sockaddr *)&this->address, sizeof(this->address)) < 0){
        perror("Failure in connect() in TCPClient.\n");
        exit(EXIT_FAILURE);
    }

}

/**********************************************************************************************
 * handleConnection - While client is still connected, get number from server and run Pollard's
 * Rho on that number. Return the value from Pollard's Rho combined with original number.
 * 
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
**********************************************************************************************/
void TCPClient::handleConnection() {
   while(1) {
      this->valread = read (client_sock, buf, 1024);
      std::cout << "number: " << buf << "\n";
      LARGEINT number = boost::lexical_cast<LARGEINT>(buf);
      LARGEINT divisor = calcPollardsRho(number);
      std::string s = boost::lexical_cast<std::string>(divisor);
      std::cout << "divisor: " << s << "\n";
      s.append(" ");
      s.append(buf);
      //clears buf
      for(int i = 0; i < 1024; i++){
        this->buf[i] = '\0';
      }
      send(this->client_sock, s.c_str(), strlen(s.c_str()), 0);
     
   }

}

/**********************************************************************************************
 * closeConnection - Your comments here
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
**********************************************************************************************/
void TCPClient::closeConn() {
    close(this->client_sock); 
}

LARGEINT TCPClient::calcPollardsRho(LARGEINT n) {
   //this means n = 1, 2 or 3. should never occur because factor() would take out all 2 and 3s
   if (n <= 3){
      return n;
   }

   // Initialize our random number generator
   srand(time(NULL));

   /**ADAPTED ALGORITHM*************************
   * Instead of X being 2-N and C being 1-N, test by using sqrt(n)
   * This is a suggest improvisation noted at this link:
   * https://www.geeksforgeeks.org/pollards-rho-algorithm-prime-factorization/
   *
   ********************************************/
   LARGEINT2X nroot = mp::sqrt(n);
   //do this to make sure there is no divide by 0 or negative number
   if(nroot <= 2) {
      nroot++;
   }
  // pick a random number from the range [2, N)
   LARGEINT2X x = (rand()%(nroot-2)) + 2;
   LARGEINT2X y = x;    // Per the algorithm

   // random number for c = [1, N)
   LARGEINT2X c = (rand()%(nroot-1)) + 1;

   LARGEINT2X d = 1;

   // Loop until either we find the gcd or gcd = 1
   while (d == 1) {
      // "Tortoise move" - Update x to f(x) (modulo n)
      // f(x) = x^2 + c f
      x = (modularPow(x, 2, n) + c + n) % n;
      // "Hare move" - Update y to f(f(y)) (modulo n)
      y = (modularPow(y, 2, n) + c + n) % n;
      y = (modularPow(y, 2, n) + c + n) % n;

      // Calculate GCD of |x-y| and tn
      LARGESIGNED2X z = (LARGESIGNED2X) x - (LARGESIGNED2X) y;
      if (z < 0)
         d = boost::math::gcd((LARGEINT2X) -z, (LARGEINT2X) n);
      else
         d = boost::math::gcd((LARGEINT2X) z, (LARGEINT2X) n);

      // If we found a divisor, factor primes out of each side of the divisor
      if ((d != 1) && (d != n)) {
         return (LARGEINT) d;

      }

   }
   return (LARGEINT) d;
}

/**
 * Function to calculate (base^exponent) % modulus, from the Geeks for Geeks article.
 *  This function is used in the calcPollardsRho().
 **/
LARGEINT2X TCPClient::modularPow(LARGEINT2X base, int exponent, LARGEINT2X modulus) {
   LARGEINT2X result = 1;

   while (exponent > 0) {

      // If the exponent is odd, calculate our results 
      if (exponent & 1) {
         result = (result * base) % modulus;
      }

      // exponent = exponent / 2 (rounded down)
      exponent = exponent >> 1;

      base = (base * base) % modulus;
   }
   return result;
}


