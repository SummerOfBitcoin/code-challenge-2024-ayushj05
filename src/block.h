#ifndef BLOCK_H
#define BLOCK_H

#include <string>
#include <queue>
#include <algorithm>
#include <ctime>
#include "script.h"
using namespace std;

extern string target;

string getMerkleRoot(vector<string> &txns_included) {
    queue<string> hashQ;
    // Push all txids into the queue in reverse order because coinbase txn is at the end
    for (int i = txns_included.size() - 1; i >= 0; i--)
        hashQ.push(txns_included[i]);

    uint32_t prev_size = hashQ.size();
    bool start = true;

    while (hashQ.size() > 1) {
        if ((start || hashQ.size() == prev_size/2) && hashQ.size() % 2 == 1) {
            hashQ.push(hashQ.back());
            prev_size = hashQ.size();
        }
        start = false;
        
        string first = hashQ.front();
        hashQ.pop();
        string second = hashQ.front();
        hashQ.pop();

        string combined = first + second;
        char sha[SHA256_DIGEST_LENGTH];
        SHA256((unsigned char*) combined.c_str(), combined.length(), (unsigned char*) sha);
        SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);
        combined = "";
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
            combined.push_back(sha[i]);

        hashQ.push(combined);
    }

    return hashQ.front();
}

string genCoinbaseTxn (int64_t reward, string wTXID_commitment) {
    string coinbase_txn = "";

    coinbase_txn += int2bin(1); // Version
    coinbase_txn += string{(int8_t) 0x00, (int8_t) 0x01}; // Marker, Flag
    coinbase_txn += int2compact(1); // Number of inputs
    
    coinbase_txn += string(32, (int8_t) 0); // Previous txid
    coinbase_txn += int2bin(0xffffffff); // Previous index (vout)
    coinbase_txn += string{(int8_t) 0x04, (int8_t) 0x03, (int8_t) 0x95, (int8_t) 0x1a, (int8_t) 0x06}; // scriptSig
    coinbase_txn += int2bin(0xffffffff); // Sequence
    
    coinbase_txn += int2compact(2); // Number of outputs
    
    coinbase_txn += int2bin(reward, IS_INT64_T); // Reward
    coinbase_txn += int2compact(0x19); // Output script length
    coinbase_txn += hexstr2bstr("76a9142c30a6aaac6d96687291475d7d52f4b469f665a688ac"); // Output script

    coinbase_txn += int2bin(0, IS_INT64_T); // Reward
    coinbase_txn += int2compact(0x26); // Output script length
    coinbase_txn += hexstr2bstr("6a24aa21a9ed");    // Output script
    coinbase_txn += wTXID_commitment;               // Output script
    coinbase_txn += hexstr2bstr("01200000000000000000000000000000000000000000000000000000000000000000");
    
    coinbase_txn += int2bin(0); // Locktime

    return coinbase_txn;
}

string genBlockHeader (vector<string> &txns_included) {
    string block_header = "";
    block_header += int2bin(32); // Version
    block_header += string(32, (int8_t) 0); // Previous block hash
    
    string merkle_root = getMerkleRoot(txns_included);
    block_header += merkle_root;

    time_t timestamp = time(nullptr);
    block_header += int2bin(timestamp);

    block_header += string{(int8_t) 255, (int8_t) 255, (int8_t) 0, (int8_t) 31}; // Bits

    return block_header;
}

string calcNonce (string blockHeader) {
    string nonce = "";
    for (int i = 0; i < 4; i++)
        nonce.push_back(0);

    for (int i = 0; i < 0x100000000; i++) {
        string tmp = blockHeader + nonce;

        char sha[SHA256_DIGEST_LENGTH];
        SHA256((unsigned char*) tmp.c_str(), tmp.length(), (unsigned char*) sha);
        SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);
        string hash = "";
        for (int i = SHA256_DIGEST_LENGTH - 1; i >= 0; i--)
            hash.push_back(sha[i]);

        if (hash < target) {
            cout << "Hash = " << bstr2hexstr(hash, hash.length()) << endl;
            return nonce;
        }
        
        // Increment nonce
        int j = 3;
        while (j >= 0 && ++nonce[j] == 0)
            j--;
    }

    return "";
}

#endif /* BLOCK_H */