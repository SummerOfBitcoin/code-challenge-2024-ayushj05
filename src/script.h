#ifndef SCRIPT_H
#define SCRIPT_H

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <jsoncpp/json/json.h>
#include "crypto.h"
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
    std::string sig = scriptSigOps[1];
    std::string pubKey = scriptSigOps[3];

    std::string pkh = scriptPubKeyOps[3];
    
    char sha[65], rpmd[41];
    sha256(pubKey.c_str(), sha);
    rpmd160(sha, rpmd);

    cout << "RIPEMD160: " << rpmd << endl;
    cout << "PKH:       " << pkh << endl;

    if (rpmd == pkh) {
        // Remove scriptsig and scriptsig_asm from txn
        // cout << "Entered" << endl;
        return true;
        for (auto &inp : txn["vin"])
            inp["scriptsig"] = inp["scriptsig_asm"] = "";
        
        char sha2[65];
        sha256(txn.toStyledString().c_str(), sha);
        sha256(sha, sha2);

        // Verify sig with pubKey and txn hash using ecdsa
        char key[pubKey.length()+1];
        sprintf(key, "%s", pubKey.c_str());
        EC_KEY* publicKey = EC_KEY_new_by_curve_name(NID_secp256k1);
        EC_KEY_set_public_key(publicKey, Hex_to_point_NID_secp256k1(key));
        if (ECDSA_verify(0, (unsigned char*) sha2, strlen(sha2), (unsigned char*) sig.c_str(), strlen(sig.c_str()), publicKey) == 1) {
            // cout << "Match!!" << endl;
            return true;
        }
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


bool verify_txn(Json::Value &txn) {
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