#include "Pollard.h"
#include <iostream>
#include <list>
#include <string>
#include <algorithm>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/integer/common_factor.hpp>


// Set up the namespace for the boost variables
namespace mp = boost::multiprecision;



// Simple constructor, just sets origional_value
Pollard::Pollard(LARGEINT input_value):origional_value(input_value) {}
// Simple destructor
Pollard::~Pollard() {}


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
bool Pollard::isPrimeBF(LARGEINT n, LARGEINT &divisor) {
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

/**********************************************************************************************
 * calcPollardsRho - Do the actual Pollards Rho calculations to attempt to find a divisor
 *
 *    Params:  n - the number to find a divisor within
 *    
 *    Returns: a divisor if found, otherwise n
 *
 *
 **********************************************************************************************/
LARGEINT Pollard::calcPollardsRho(LARGEINT n) {

   if (n <= 3){
      return n;
   }

   // Initialize our random number generator
   srand(time(NULL));

   // pick a random number from the range [2, N)
   LARGEINT2X x = (rand()%(n-2)) + 2;
   LARGEINT2X y = x;    // Per the algorithm

   // random number for c = [1, N)
   LARGEINT2X c = (rand()%(n-1)) + 1;

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
* factor - Calculates a single prime of the given number and recursively calls
*          itself to continue calculating primes of the remaining number. Variation
*          on the algorithm by Yash Varyani on GeeksForGeeks. Uses a single
*          process 
**/
void Pollard::factor(){

   // First, take care of the '2' factors
   LARGEINT newval = getOrigionalValue();
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

   // Now use Pollards Rho to figure out the rest. As it's stochastic, we don't know
   // how long it will take to find an answer. Should return the final two primes
   factor(newval); 
}


/**
* factor - same as above function, but can be iteratively called as numbers are
*          discovered as the number n can be passed in
**/
void Pollard::factor(LARGEINT n) {
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
        LARGEINT d = calcPollardsRho(n);
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

/**
 * Function to calculate (base^exponent) % modulus, from the Geeks for Geeks article.
 *  This function is used in the calcPollardsRho().
 **/
LARGEINT2X Pollard::modularPow(LARGEINT2X base, int exponent, LARGEINT2X modulus) {
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

/**
 * Print all of the numbers in the prime list to the terminal
 **/
void Pollard::printPrimes(){
    std::cout << "The list of prime factors for the input number: " << std::endl;
    std::cout << origional_value << std::endl; 

    for(auto const& p : primes){
        std::cout << p << ", "; 
    }
    std::cout << std::endl; 
    
    
}

/**
 * Get a number from the terminal and perform pollards rho on it. 
  
int main(){
    LARGEINT find("463463374607431768211451");

    Pollard solver = Pollard(find);
    solver.factor();
    solver.printPrimes();
    
    return 0;
}
**/
