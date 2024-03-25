#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>
#include <cstring>
#include <openssl/ripemd.h>
#include <openssl/sha.h>

void sha256(const char *string, char outputBuffer[65])
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, string, strlen(string));
    SHA256_Final(hash, &sha256);
    int i = 0;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}

void rpmd160(const char *string, char outputBuffer[41])
{
    unsigned char hash[RIPEMD160_DIGEST_LENGTH];
    RIPEMD160_CTX ripemd;
    RIPEMD160_Init(&ripemd);
    RIPEMD160_Update(&ripemd, string, strlen(string));
    RIPEMD160_Final(hash, &ripemd);
    int i = 0;
    for(i = 0; i < RIPEMD160_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[40] = 0;
}

#endif // CRYPTO_H