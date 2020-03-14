#ifndef POLLARD_H
#define POLLARD_H

#include <list>
#include <boost/multiprecision/cpp_int.hpp>

// Set up the namespace for the boost variables
namespace mp = boost::multiprecision;

/**
 * Using this notation will make scaling up (and down for initial testing)
 * the solution to handle 128 bit and 256 bit numbers much easier.
 * 
 * int = 32 bit
 * long long int = 64 bits
 * mp::int128_t = 128 bits
 * mp::int256_t = 256 bits
 * mp::int512_t = 512 bits, but 256 bits is the positive (unsigned) part
 **/

/* "UNSIGNED int type to hold original value and calculations" */
#define LARGEINT mp::uint128_t

/* "UNSIGNED int twice as large as LARGEINT (bit-wise)" */
#define LARGEINT2X mp::uint256_t

/* "SIGNED int made of twice the bits as LARGEINT2X" */
#define LARGESIGNED2X mp::int512_t

/**
 * Src:
 * The DivFinder code provided by the instructor and 
 * https://www.geeksforgeeks.org/pollards-rho-algorithm-prime-factorization/
 * 
 * Pollard's Rho implementation for finding prime numbers. 
 **/
class Pollard{
    public:
        Pollard(LARGEINT input_value);
        virtual ~Pollard();

        bool isPrimeBF(LARGEINT n, LARGEINT &divisor);

        LARGEINT calcPollardsRho(LARGEINT n);

        // Simple getter for origional_value field
        LARGEINT getOrigionalValue() {return origional_value;}

        void factor();

        void printPrimes();

   protected:
        // Recursive factor call, only used in factor()
        void factor(LARGEINT n);

        LARGEINT2X modularPow(LARGEINT2X base, int exponent, LARGEINT2X modulus);

    private:
        // What we are trying to factor
        LARGEINT origional_value;

        // The list of all the prime factors of origional_value
        std::list<LARGEINT> primes; 

        // If factor has been called on a number x times, check to see if the number is prime
        const unsigned int primecheck_depth = 10;
    
};

#endif