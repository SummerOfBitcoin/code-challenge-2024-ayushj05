#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <iostream>
#include <string>
#include <algorithm>
#include <cmath>
#include <jsoncpp/json/json.h>
#include <openssl/sha.h>
using namespace std;

#define IS_UINT32_T 0
#define IS_INT64_T 1

extern string identifyScriptType(std::vector<std::string> &ops);
extern std::vector<std::string> getOps(std::string asmScript);

typedef basic_string<unsigned char> ustring;

uint8_t hex2int(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return -1;
}

// JUST FOR TESTING => MAY REMOVE
// Convert byte (of the form 0x0X) to hex
char byte2hex(char num){
    if (num >= 0 && num <= 9) return num + '0';
    else return num - 10 + 'a';
}

// JUST FOR TESTING => MAY REMOVE
// Convert byte string to hex string
string bstr2hexstr(string byte_string, size_t len){
    string hex_string = "";
    for (int i = 0; i < len; i++){
        hex_string.push_back(byte2hex(((unsigned char) byte_string[i])/16));
        hex_string.push_back(byte2hex(((unsigned char) byte_string[i])%16));
    }
    return hex_string;
}

// Convert hex string to byte string
// @param reverse: For inverting txn id for txn serialization
string hexstr2bstr (string hex_string, bool reverse = false) {
    string byte_string = "";
    if (reverse) {
        for (int i = hex_string.length() - 2; i >= 0; i-=2)
            byte_string.push_back((hex2int(hex_string[i]) << 4) + hex2int(hex_string[i+1]));
    }
    else {
        for (int i = 0; i < hex_string.length(); i+=2)
            byte_string.push_back((hex2int(hex_string[i]) << 4) + hex2int(hex_string[i+1]));
    }
    return byte_string;
}

// Convert integer to hex string (binary format) as little-endian
string int2bin (int64_t num, int type = IS_UINT32_T) {
    string bin = "";

    if (num == 0) {
        for (int i=0; i<4*type + 4; i++)
            bin.push_back(0);
        return bin;
    }

    // 2's complement
    if (num < 0)
        num = (UINT64_MAX + num) + 1;

    while (num != 0) {
        bin.push_back(num % 256);
        num /= 256;
    }
    for (int i = bin.length(); i < 4*(type + 1); i++)
        bin.push_back(0);

    return bin;
}

// Convert integer to CompactSize uint (binary format)
string int2compact (uint64_t num) {
    string comp = "";
    
    if (num == 0){
        comp.push_back(0);
        return comp;
    }

    if (num >= 253 && num <= 65535) {
        comp.push_back(253);
        for (int i = 0; i < 1 - log2(num)/8; i++)
            comp.push_back(0);
    }
    else if (num >= 65536 && num <= 4294967295) {
        comp.push_back(254);
        for (int i = 0; i < 3 - log2(num)/8; i++)
            comp.push_back(0);
    }
    else if (num >= 4294967296){
        comp.push_back(255);
        for (int i = 0; i < 7 - log2(num)/8; i++)
            comp.push_back(0);
    }
    
    while (num > 0) {
        comp.push_back(num % 256);
        num /= 256;
    }

    return comp;
}

string serialize_txn (Json::Value txn, bool isCleared = false) {
    string ser_txn = "";
    
    ser_txn += int2bin(txn["version"].asUInt());
    ser_txn += int2compact(txn["vin"].size());
    
    for (auto &inp : txn["vin"]) {
        // outpoint
        string txid = hexstr2bstr(inp["txid"].asString());
        reverse(txid.begin(), txid.end());
        ser_txn += txid;
        ser_txn += int2bin(inp["vout"].asUInt());

        if (isCleared) {
            ser_txn += int2compact(inp["prevout"]["scriptpubkey"].asString().length()/2);
            ser_txn += hexstr2bstr(inp["prevout"]["scriptpubkey"].asString());
        }
        else {
            ser_txn += int2compact(inp["scriptsig"].asString().length()/2);
            ser_txn += hexstr2bstr(inp["scriptsig"].asString());
        }
        ser_txn += int2bin(inp["sequence"].asUInt());
    }

    ser_txn += int2compact(txn["vout"].size());

    for (auto &out : txn["vout"]) {
        ser_txn += int2bin(out["value"].asInt64(), IS_INT64_T);
        ser_txn += int2compact(out["scriptpubkey"].asString().length()/2);
        ser_txn += hexstr2bstr(out["scriptpubkey"].asString());
    }

    ser_txn += int2bin(txn["locktime"].asUInt());

    return ser_txn;
}

string serialize_segwit_1 (Json::Value txn) {
    string serialized = "";
    
    serialized += int2bin(txn["version"].asUInt());
    
    string inputs = "";
    for (auto &inp : txn["vin"]) {
        // outpoint
        string txid = hexstr2bstr(inp["txid"].asString());
        reverse(txid.begin(), txid.end());
        inputs += txid;
        inputs += int2bin(inp["vout"].asUInt());
    }
    char sha[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*) inputs.c_str(), inputs.length(), (unsigned char*) sha);
    SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        serialized.push_back(sha[i]);

    string sequences = "";
    for (auto &inp : txn["vin"]) {
        sequences += int2bin(inp["sequence"].asUInt());
    }
    SHA256((unsigned char*) sequences.c_str(), sequences.length(), (unsigned char*) sha);
    SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        serialized.push_back(sha[i]);

    return serialized;
}


string serialize_segwit_inp (Json::Value inp, string scriptType) {
    string ser_inp = "";
    
    ser_inp = hexstr2bstr(inp["txid"].asString());
    reverse(ser_inp.begin(), ser_inp.end());
    ser_inp += int2bin(inp["vout"].asUInt());

    // Need to find the PKH from prevout's scriptpubkey
    string scriptcode = "1976a914";
    if (scriptType == "p2sh") {
        vector<string> ops = getOps(inp["inner_redeemscript_asm"].asString());
        if (identifyScriptType(ops) == "p2wpkh")
            scriptcode += ops[2];
    }
    else if (scriptType == "p2wpkh")
        scriptcode += inp["prevout"]["scriptpubkey"].asString().substr(4);
    scriptcode += "88ac";
    ser_inp += hexstr2bstr(scriptcode);

    ser_inp += int2bin(inp["prevout"]["value"].asInt64(), IS_INT64_T);

    ser_inp += int2bin(inp["sequence"].asUInt());

    return ser_inp;
}

string serialize_segwit_2 (Json::Value txn) {
    string serialized = "";

    for (auto &out : txn["vout"]) {
        serialized += int2bin(out["value"].asInt64(), IS_INT64_T);
        serialized += int2compact(out["scriptpubkey"].asString().length()/2);
        serialized += hexstr2bstr(out["scriptpubkey"].asString());
    }
    char sha[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*) serialized.c_str(), serialized.length(), (unsigned char*) sha);
    SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);
    serialized = "";
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        serialized.push_back(sha[i]);

    serialized += int2bin(txn["locktime"].asUInt());

    return serialized;
}

#endif // SERIALIZE_H