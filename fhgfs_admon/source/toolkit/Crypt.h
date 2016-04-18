#ifndef CRYPT_H_
#define CRYPT_H_

/* small class with static functions to apply cryptographic or hashing functions to strings */

#include <common/Common.h>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <openssl/md5.h>

#define rotateleft(x,n) ((x<<n) | (x>>(32-n)))
#define rotateright(x,n) ((x>>n) | (x<<(32-n)))


class Crypt
{
public:
   Crypt() { };

   static std::string DoSHA(std::string str1);
   static std::string DoMD5(std::string str);
};

#endif /*CRYPT_H_*/
