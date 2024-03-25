#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <iostream>
#include <string>
#include <cmath>
#include <jsoncpp/json/json.h>
using namespace std;

extern uint8_t hex2int(char ch);

#define IS_UINT32_T 0
#define IS_INT64_T 1

typedef basic_string<unsigned char> ustring;

// JUST FOR TESTING => MAY REMOVE
// Convert byte (of the form 0x0X) to hex
char byte2hex(char num){
    if (num >= 0 && num <= 9) return num + '0';
    else return num - 10 + 'a';
}

// JUST FOR TESTING => MAY REMOVE
// Convert byte string to hex string
string bstr2hexstr(string byte_string){
    string hex_string = "";
    for (int i = 0; i < byte_string.length(); i++){
        hex_string.push_back(byte2hex(((unsigned char) byte_string[i])/16));
        hex_string.push_back(byte2hex(((unsigned char) byte_string[i])%16));
    }
    return hex_string;
}

string hexstr2bstr (string hex_string) {
    string byte_string = "";
    for (int i = 0; i < hex_string.length(); i+=2)
        byte_string.push_back((hex2int(hex_string[i]) << 4) + hex2int(hex_string[i+1]));
    return byte_string;
}

// Convert integer to hex string (binary format)
string int2hex (int64_t num, int type = IS_UINT32_T) {
    string hex = "";

    if (num == 0) {
        for (int i=0; i<4*type + 4; i++)
            hex.push_back(0);
        return hex;
    }

    // 2's complement
    if (num < 0)
        num = (UINT64_MAX + num) + 1;

    for (int i = 0; i < 4*type + 3 - log2(abs(num))/8; i++)
        hex.push_back(0);
    while (num != 0) {
        hex.push_back(abs(num) % 256);
        num /= 256;
    }

    return hex;
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

string sertialize_txn (Json::Value txn, bool isCleared = false) {
    string ser_txn = "";
    
    ser_txn += int2hex(txn["version"].asUInt());
    ser_txn += int2compact(txn["vin"].size());
    
    for (auto &inp : txn["vin"]) {
        // outpoint
        ser_txn += hexstr2bstr(inp["txid"].asString());
        ser_txn += int2hex(inp["vout"].asUInt());

        if (isCleared)
            ser_txn += int2compact(0);
        else {
            ser_txn += int2compact(inp["scriptsig"].asString().length()/2);
            ser_txn += hexstr2bstr(inp["scriptsig"].asString());
        }
        ser_txn += int2hex(inp["sequence"].asUInt());
    }

    ser_txn += int2compact(txn["vout"].size());

    for (auto &out : txn["vout"]) {
        ser_txn += int2hex(out["value"].asInt64(), IS_INT64_T);
        ser_txn += int2compact(out["scriptpubkey"].asString().length()/2);
        ser_txn += hexstr2bstr(out["scriptpubkey"].asString());
    }

    ser_txn += int2hex(txn["locktime"].asUInt());

    return ser_txn;
}

#endif // SERIALIZE_H