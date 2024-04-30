#ifndef SCRIPT_H
#define SCRIPT_H

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <stack>
#include <jsoncpp/json/json.h>
#include "serialize.h"
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/ecdsa.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/err.h>
using namespace std;

int total = 0, valid_txns = 0, invalid_txns = 0;
extern set<string> inputs_used;
string serialized_txn = "", serialized_segwit_1 = "", serialized_segwit_inp = "", serialized_segwit_2 = "";

// Generates double sha256 hash of the input data
// Output in byte format
string hash256(string data) {
    string hash = "";
    char sha[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*) data.c_str(), data.length(), (unsigned char*) sha);
    SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        hash.push_back(sha[i]);
    return hash;
}

// Get vector of operations from the script
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


/*
    Verifies the signature in an input
    @param serialized_txn: Serialized transaction in byte string format
    @param pubKey: Public key in hex string format
    @param signature: Signature in hex string format
*/
bool verify_sig (string pubKey, std::string signature) {
    // Add the SIGHASH flag to the serialized transaction
    serialized_txn.push_back((hex2int(signature[signature.length()-2]) << 4) + hex2int(signature[signature.length()-1]));
    for (int i = 0; i < 3; i++)
        serialized_txn.push_back(0);

    // Remove the SIGHASH flag from the signature
    signature.pop_back();
    signature.pop_back();

    // Convert signature to byte format
    char sig_byte[signature.length()/2];
    for (int i = 0; i < signature.length(); i+=2)
        sig_byte[i/2] = (hex2int(signature[i]) << 4) + hex2int(signature[i+1]);

    // Calculate double sha256 hash of the transaction
    char sha[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*) serialized_txn.c_str(), serialized_txn.length(), (unsigned char*) sha);
    SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);

    // Verify sig with pubKey and txn hash using ecdsa
    EC_KEY* publicKey = EC_KEY_new_by_curve_name(NID_secp256k1);
    EC_KEY_set_public_key(publicKey, Hex_to_point_NID_secp256k1((char*) pubKey.c_str()));
    int verified = ECDSA_verify(0, (unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sig_byte, signature.length()/2, publicKey);
    
    if (verified == 1) {
        EC_KEY_free(publicKey); // Free allocated memory
        return true;
    }
    else if (verified == -1) {
        // Get error
        char errorString[256];
        ERR_error_string(ERR_get_error(), errorString);
        cout << errorString << endl;
    }
    EC_KEY_free(publicKey); // Free allocated memory

    return false;
}

bool exec_op (string op, stack<string> &stk, Json::Value &txn, int &bytes_to_push) {
    if (op == "OP_0") {
        stk.push("0");
    }
    // Push number of bytes to push to stack
    else if (op.substr(0, min(op.length(), (size_t) 10)) == "OP_PUSHNUM") {
        stk.push(op.substr(11));
    }
    else if (bytes_to_push > 0) {
        stk.push(op.substr(0, bytes_to_push*2));
        bytes_to_push = 0;
    }
    // Push "bytes_to_push" bytes to stack
    else if (op.substr(0, 12) == "OP_PUSHBYTES") {
        bytes_to_push = stoi(op.substr(13));
    }
    else if (op.substr(0, 11) == "OP_PUSHDATA") {
        bytes_to_push = -stoi(op.substr(11));
    }
    // -ve value of bytes_to_push means that we have to read the first "bytes_to_push" bytes to get the number of bytes to push
    else if (bytes_to_push < 0) {
        int num_bytes = 0;      // Number of bytes to push
        for (int i=0; i<-bytes_to_push*2; i+=2)
            num_bytes += ((hex2int(op[i]) << 4) + hex2int(op[i+1])) << (4*i);

        stk.push(op.substr(0, -num_bytes*2));
        bytes_to_push = 0;
    }
    // Duplicating top stack item
    else if (op == "OP_DUP") {
        stk.push(stk.top());
    }
    // Hashing the input twice: first with SHA-256 and then with RIPEMD-160
    else if (op == "OP_HASH160") {
        char top_byte[stk.top().length()/2];
        for (int i = 0; i < stk.top().length(); i+=2)
            top_byte[i/2] = (hex2int(stk.top()[i]) << 4) + hex2int(stk.top()[i+1]);

        char sha[SHA256_DIGEST_LENGTH], rpmd[RIPEMD160_DIGEST_LENGTH];
        SHA256((unsigned char*) top_byte, sizeof(top_byte), (unsigned char*) sha);
        RIPEMD160((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) rpmd);
        stk.pop();
        stk.push(bstr2hexstr(rpmd, sizeof(rpmd)));
    }
    // Comparing the top two stack items. If they are not equal, the script fails
    else if (op == "OP_EQUALVERIFY") {
        string top = stk.top();
        stk.pop();
        if (top == stk.top())
            stk.pop();
        else {
            stk.push("FALSE");
            return false;
        }
    }
    // Verifying txn against signature and public key
    else if (op == "OP_CHECKSIG") {
        string pubKey = stk.top();
        stk.pop();
        string sig = stk.top();
        stk.pop();
        if (!verify_sig(pubKey, sig)){
            stk.push("FALSE");
            return false;
        }
    }
    // Verifying txn against signature and public key. If not valid, the script fails
    else if (op == "OP_CHECKSIGVERIFY") {
        string pubKey = stk.top();
        stk.pop();
        string sig = stk.top();
        stk.pop();
        if (!verify_sig(pubKey, sig))
            return false;
    }
    // Comparing the top two stack items. If they are not equal, the script fails
    else if (op == "OP_EQUAL") {
        string top = stk.top();
        stk.pop();
        if (top == stk.top()) {
            stk.pop();
            stk.push("TRUE");
        }
        else {
            stk.pop();
            stk.push("FALSE");
        }
    }
    // Marks transaction as invalid if top stack value is not true
    else if (op == "OP_VERIFY") {
        if (stk.top() == "TRUE")
            stk.pop();
        else
            return false;
    }
    // For each signature and public key pair, OP_CHECKSIG is executed. If all signatures are valid, the script returns true
    else if (op == "OP_CHECKMULTISIG") {        
        int num_pubKeys = stoi(stk.top());
        stk.pop();
        vector<string> pubKeys;
        for (int i = 0; i < num_pubKeys; i++) {
            pubKeys.push_back(stk.top());
            stk.pop();
        }

        int num_sigs = stoi(stk.top());
        stk.pop();
        vector<string> sigs;
        for (int i = 0; i < num_sigs; i++) {
            sigs.push_back(stk.top());
            stk.pop();
        }

        int j = 0;
        for (int i = 0; i < num_sigs; i++) {
            // Checked all the pubKeys but still have some signatures left
            if (j >= num_pubKeys) {
                stk.push("FALSE");
                return false;
            }
            // Public key does not match the signature, so check with next public key
            else if (!verify_sig(pubKeys[j], sigs[i])) {
                i--;
                j++;
            }
            else
                j++;
        }
    }
    // Dropping the top stack item
    else if (op == "OP_DROP") {
        stk.pop();
    }
    // Pushing the number of stack items to stack
    else if (op == "OP_DEPTH") {
        stk.push(to_string(stk.size()));
    }
    // Invalid operation
    else {
        return false;
    }
    
    return true;
}

// Identifies the type of script
string identifyScriptType(std::vector<std::string> &ops) {
    if (ops[0] == "OP_DUP" && ops[1] == "OP_HASH160" && ops[2] == "OP_PUSHBYTES_20" && ops[4] == "OP_EQUALVERIFY" && ops[5] == "OP_CHECKSIG")
        return "p2pkh";
    else if (ops[0] == "OP_HASH160" && ops[1] == "OP_PUSHBYTES_20" && ops[3] == "OP_EQUAL")
        return "p2sh";
    else if (ops[0] == "OP_0" && ops[1] == "OP_PUSHBYTES_20")
        return "p2wpkh";
    else if (ops[0] == "OP_0" && ops[1] == "OP_PUSHBYTES_32")
        return "p2wsh";
    else if (ops[0] == "OP_PUSHNUM_1" && ops[1] == "OP_PUSHBYTES_32")
        return "p2tr";
    else
        return "Invalid";
}

// Verify a p2pkh script
bool p2pkh_verify(std::vector<std::string> &scriptPubKeyOps, std::vector<std::string> &scriptSigOps, Json::Value &txn) {
    stack<string> stk;
    int bytes_to_push = 0;

    for (string op : scriptSigOps){
        bool return_val = exec_op(op, stk, txn, bytes_to_push);
        if (!return_val)
            return false;
    }

    for (string op : scriptPubKeyOps){
        bool return_val = exec_op(op, stk, txn, bytes_to_push);
        if (!return_val)
            return false;
    }

    return true;
}

// Verify a p2wpkh script
bool p2wpkh_verify(std::vector<std::string> scriptPubKeyOps, std::vector<std::string> witness, Json::Value txn) {
    stack<string> stk;
    int bytes_to_push = 0;

    stk.push(witness[0]);
    stk.push(witness[1]);

    bool return_val = exec_op("OP_DUP", stk, txn, bytes_to_push);
    return_val = exec_op("OP_HASH160", stk, txn, bytes_to_push);

    for (string op : scriptPubKeyOps){
        // Not putting 0 on stack because it will fail OP_EQUALVERIFY
        if (op == "OP_0")
            continue;
        return_val = exec_op(op, stk, txn, bytes_to_push);
        if (!return_val)
            return false;
    }

    return_val = exec_op("OP_EQUALVERIFY", stk, txn, bytes_to_push);
    if (!return_val)
        return false;
    
    return_val = exec_op("OP_CHECKSIG", stk, txn, bytes_to_push);
    if (!return_val)
        return false;

    return true;
}

// Verify a p2wsh script
bool p2wsh_verify(std::vector<std::string> &scriptPubKeyOps, std::vector<std::string> &witness, std::vector<std::string> &witnessScriptOps, Json::Value &txn) {
    stack<string> stk;
    int bytes_to_push = 0;

    for (string op : scriptPubKeyOps){
        bool return_val = exec_op(op, stk, txn, bytes_to_push);
        if (!return_val)
            return false;
    }

    stk.push(witness.back());

    bool return_val = exec_op("OP_EQUALVERIFY", stk, txn, bytes_to_push);
    if (!return_val)
        return false;
    
    for (int i = 1; i < witness.size() - 1; i++)
        stk.push(witness[i]);
    
    for (string op : witnessScriptOps){
        return_val = exec_op(op, stk, txn, bytes_to_push);
        if (!return_val)
            return false;
    }

    return true;
}

// Verify a p2tr script
bool p2tr_verify(std::vector<std::string> &scriptPubKeyOps, std::vector<std::string> &scriptSigOps, Json::Value &txn) {
    return true;
}

// Verify a multisig script
bool multisig_verify(std::vector<std::string> &redeemScriptOps, std::vector<std::string> &scriptSigOps, Json::Value &txn) {
    stack<string> stk;
    int bytes_to_push = 0;

    for (string op : scriptSigOps){
        bool return_val = exec_op(op, stk, txn, bytes_to_push);
        if (!return_val)
            return false;
    }

    for (string op : redeemScriptOps){
        bool return_val = exec_op(op, stk, txn, bytes_to_push);
        if (!return_val)
            return false;
    }

    return true;
}

// Verify a p2sh script
bool p2sh_verify(std::vector<std::string> &scriptPubKeyOps, std::vector<std::string> &scriptSigOps, std::vector<std::string> &redeemScriptOps, Json::Value &txn, Json::Value &inp) {
    stack<string> stk;
    int bytes_to_push = 0;

    // Push values from scriptSig to stack
    for (string op : scriptSigOps){
        bool return_val = exec_op(op, stk, txn, bytes_to_push);
        if (!return_val)
            return false;
    }

    // Push values from pubKeyScript to stack
    for (string op : scriptPubKeyOps){
        bool return_val = exec_op(op, stk, txn, bytes_to_push);
        if (!return_val)
            return false;
    }

    if (stk.top() == "FALSE")
        return false;
    
    // Identify the type of redeem script
    string redeemScriptType = identifyScriptType(redeemScriptOps);

    if (redeemScriptType == "p2wpkh") {
        std::vector<std::string> witness;
        for (auto &wtns : inp["witness"])
            witness.push_back(wtns.asString());
        return p2wpkh_verify(redeemScriptOps, witness, txn);
    }
    else if (redeemScriptType == "p2wsh") {
        std::vector<std::string> witness;
        for (auto &wtns : inp["witness"])
            witness.push_back(wtns.asString());
        std::vector<std::string> redeemScriptOps = getOps(inp["redeemscript_asm"].asString());
        return p2wsh_verify(redeemScriptOps, witness, redeemScriptOps, txn);
    }
    // Invalid means it is a multisig script
    else if (redeemScriptType == "Invalid")
        return multisig_verify(redeemScriptOps, scriptSigOps, txn);
    
    return true;
}


/*
The main function to verify a transaction
Returns -1 if the transaction is invalid
Else returns the transaction fees
*/
int verify_txn(Json::Value &txn) {
    bool valid = true;

    // Check if sum of input values is greater than sum of output values
    uint64_t sum_input = 0, sum_output = 0;
    for (auto &in : txn["vin"])
        sum_input += in["prevout"]["value"].asUInt64();
    for (auto &out : txn["vout"])
        sum_output += out["value"].asUInt64();
    if (sum_input < sum_output) {
        return -1;
    }

    // Validate each input
    set<string> txn_inputs;
    for (auto &inp : txn["vin"]) {
        // Check if the input is already used either in any previous txn or in the current txn (double spending)
        if (inputs_used.find(inp["txid"].asString()) != inputs_used.end() || txn_inputs.find(inp["txid"].asString()) != txn_inputs.end())
            return -1;
        txn_inputs.insert(inp["txid"].asString());

        std::vector<std::string> scriptPubKeyOps = getOps(inp["prevout"]["scriptpubkey_asm"].asString());
        std::string scriptPubKeyType = inp["prevout"]["scriptpubkey_type"].asString();

        if (scriptPubKeyType == "p2pkh") {
            serialized_txn = serialize_txn(txn, true);
            std::vector<std::string> scriptSigOps = getOps(inp["scriptsig_asm"].asString());
            valid = p2pkh_verify(scriptPubKeyOps, scriptSigOps, txn);
        }
        else if (scriptPubKeyType == "p2sh") {
            serialized_txn = serialize_segwit_1(txn) + serialize_segwit_inp(inp, "p2sh") + serialize_segwit_2(txn);
            std::vector<std::string> scriptSigOps = getOps(inp["scriptsig_asm"].asString());
            std::vector<std::string> redeemScriptOps = getOps(inp["inner_redeemscript_asm"].asString());
            valid = p2sh_verify(scriptPubKeyOps, scriptSigOps, redeemScriptOps, txn, inp);
        }
        else if (scriptPubKeyType == "v0_p2wpkh") {
            serialized_txn = serialize_segwit_1(txn) + serialize_segwit_inp(inp, "p2wpkh") + serialize_segwit_2(txn);
            std::vector<std::string> witness;
            for (auto &wtns : inp["witness"])
                witness.push_back(wtns.asString());
            valid = p2wpkh_verify(scriptPubKeyOps, witness, txn);
        }
        else if (scriptPubKeyType == "v0_p2wsh") {
            serialized_txn = serialize_segwit_1(txn) + serialize_segwit_inp(inp, "p2wsh") + serialize_segwit_2(txn);
            std::vector<std::string> witness;
            for (auto &wtns : inp["witness"])
                witness.push_back(wtns.asString());
            std::vector<std::string> witnessScriptOps = getOps(inp["inner_witnessscript_asm"].asString());
            valid = p2wsh_verify(scriptPubKeyOps, witness, witnessScriptOps, txn);
        }
        else if (scriptPubKeyType == "v1_p2tr") {
            std::vector<std::string> witness;
            for (auto &wtns : inp["witness"])
                witness.push_back(wtns.asString());
            valid = p2tr_verify(scriptPubKeyOps, witness, txn);
        }

        if (!valid)
            return -1;
    }

    // Valid transaction, hence add the inputs to the set
    for (auto &inp : txn["vin"])
        inputs_used.insert(inp["txid"].asString());

    return sum_input - sum_output;
}

#endif // SCRIPT_H