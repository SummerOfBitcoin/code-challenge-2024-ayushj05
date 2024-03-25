#ifndef SCRIPT_H
#define SCRIPT_H

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <jsoncpp/json/json.h>
#include "crypto.h"
#include "serialize.h"
#include <openssl/ecdsa.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
using namespace std;

std::vector<std::string> getOps(std::string asmScript) {
    std::vector<std::string> ops;

    while (asmScript.find(" ") != std::string::npos) {
        std::string op = asmScript.substr(0, asmScript.find(" "));
        ops.push_back(op);
        asmScript = asmScript.substr(asmScript.find(" ") + 1);
    }
    ops.push_back(asmScript);

    return ops;
}

EC_POINT* Hex_to_point_NID_secp256k1(char* str) {
   EC_KEY* ecKey = EC_KEY_new_by_curve_name(NID_secp256k1);
   const EC_GROUP* ecGroup = EC_KEY_get0_group(ecKey);
   EC_POINT* ecPoint = EC_POINT_hex2point(ecGroup, str, NULL, NULL);
   return ecPoint;
}


bool p2pkh_verify(std::vector<std::string> scriptPubKeyOps, std::vector<std::string> scriptSigOps, Json::Value txn) {
    std::string pubKey = scriptSigOps[3];
    std::string pkh = scriptPubKeyOps[3];
    
    unsigned char sha[64], rpmd[40];
    sha256(pubKey.c_str(), (char*) sha);
    rpmd160((char*) sha, (char*) rpmd);

    if ((char*)rpmd == pkh) {
        // Clear scriptsig and scriptsig_asm from transaction
        // int idx = txn_str.find("\"scriptsig\"");
        // txn_str = txn_str.substr(0, idx + 14) + txn_str.substr(idx + 228);
        // idx = txn_str.find("\"scriptsig_asm\"");
        // txn_str = txn_str.substr(0, idx + 18) + txn_str.substr(idx + 261);

        // int idx = txn_str.find("\"scriptsig\"");
        // txn_str = txn_str.substr(0, idx-1) + txn_str.substr(idx + 237);
        // idx = txn_str.find("\"scriptsig_asm\"");
        // txn_str = txn_str.substr(0, idx-1) + txn_str.substr(idx + 268);

        string serialized_txn = sertialize_txn(txn, true);
        cout << "Size in bytes = " << serialized_txn.length() << "\nSerialized txn in hex: " << bstr2hexstr(serialized_txn) << endl;
        unsigned char str[serialized_txn.length()/2];
        for (int i = 0; i < serialized_txn.length(); i+=2)
            str[i/2] = (hex2int(serialized_txn[i]) << 4) + hex2int(serialized_txn[i+1]);
        
        unsigned char sha2[64];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, str, sizeof(str));
        SHA256_Final(sha, &sha256);

        SHA256_Init(&sha256);
        SHA256_Update(&sha256, sha, sizeof(sha));
        SHA256_Final(sha2, &sha256);

        // SHA256((unsigned char*) txn_str.c_str(), sizeof(txn_str.c_str()), sha);
        // SHA256(sha, sizeof(sha), sha2);

        // Verify sig with pubKey and txn hash using ecdsa
        char key[pubKey.length() + 1];
        sprintf(key, "%s", pubKey.c_str());

        EC_KEY* publicKey = EC_KEY_new_by_curve_name(NID_secp256k1);
        EC_KEY_set_public_key(publicKey, Hex_to_point_NID_secp256k1(key));

        unsigned char sig[scriptSigOps[1].length()/2];
        for (int i = 0; i < scriptSigOps[1].length(); i+=2)
            sig[i/2] = (hex2int(scriptSigOps[1][i]) << 4) + hex2int(scriptSigOps[1][i+1]);
        
        unsigned char sha_hex[sizeof(sha2)/2];
        for (int i = 0; i < sizeof(sha2); i+=2)
            sha_hex[i/2] = (hex2int(sha2[i]) << 4) + hex2int(sha2[i+1]);
        
        if (ECDSA_verify(0, sha_hex, sizeof(sha_hex), sig, sizeof(sig), publicKey) == 1) {
            cout << "Match!!" << endl;
            EC_KEY_free(publicKey); // Free allocated memory
            return true;
        }
        EC_KEY_free(publicKey); // Free allocated memory
    }

    return false;
}


bool p2sh_verify(std::vector<std::string> scriptPubKeyOps, std::vector<std::string> scriptSigOps, Json::Value txn) {
    
}


bool p2wpkh_verify(std::vector<std::string> scriptPubKeyOps, std::vector<std::string> scriptSigOps, Json::Value txn) {
    
}


bool p2wsh_verify(std::vector<std::string> scriptPubKeyOps, std::vector<std::string> scriptSigOps, Json::Value txn) {
    
}


bool p2tr_verify(std::vector<std::string> scriptPubKeyOps, std::vector<std::string> scriptSigOps, Json::Value txn) {
    
}


bool verify_txn(Json::Value &txn, std::string txn_str) {
    bool valid = true;

    for (auto &inp : txn["vin"]) {
        std::vector<std::string> scriptPubKeyOps = getOps(inp["prevout"]["scriptpubkey_asm"].asString());
        std::vector<std::string> scriptSigOps = getOps(inp["scriptsig_asm"].asString());
        std::string scriptPubKeyType = inp["prevout"]["scriptpubkey_type"].asString();

        if (scriptPubKeyType == "p2pkh")
            valid = p2pkh_verify(scriptPubKeyOps, scriptSigOps, txn);
        else if (scriptPubKeyType == "p2sh")
            valid = p2sh_verify(scriptPubKeyOps, scriptSigOps, txn);
        else if (scriptPubKeyType == "p2wpkh")
            valid = p2wpkh_verify(scriptPubKeyOps, scriptSigOps, txn);
        else if (scriptPubKeyType == "p2wsh")
            valid = p2wsh_verify(scriptPubKeyOps, scriptSigOps, txn);
        else if (scriptPubKeyType == "p2tr")
            valid = p2tr_verify(scriptPubKeyOps, scriptSigOps, txn);
        else
            return false;
        
        if (!valid)
            return false;
    }

    return true;
}

#endif // SCRIPT_H