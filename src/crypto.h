#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>
#include <cstring>
#include <openssl/ripemd.h>
#include <openssl/sha.h>

int hex2int(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return -1;
}

void sha256(const char *string, char outputBuffer[65], bool txn = false)
{
    unsigned char hash[SHA256_DIGEST_LENGTH], str[strlen(string)/2];
    for (int i = 0; i < strlen(string); i+=2)
        str[i/2] = (hex2int(string[i]) << 4) + hex2int(string[i+1]);

    SHA256(str, sizeof(str), hash);
    int i = 0;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}

void rpmd160(const char *string, char outputBuffer[41])
{
    unsigned char hash[RIPEMD160_DIGEST_LENGTH], str[strlen(string)/2];
    for (int i = 0; i < strlen(string); i+=2)
        str[i/2] = (hex2int(string[i]) << 4) + hex2int(string[i+1]);
    
    RIPEMD160(str, sizeof(str), hash);
    int i = 0;
    for(i = 0; i < RIPEMD160_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[40] = 0;
}

#endif // CRYPTO_H