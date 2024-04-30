#include <iostream>
#include <jsoncpp/json/json.h>
#include <fstream>
#include <filesystem>
#include <set>
#include <sstream>
#include "script.h"
#include "block.h"
using namespace std;

set<string> inputs_used;
string target = string(2, 0) + string(2, 255) + string(28, 0);

void mine() {
    // Vector to store all transactions in the mempool
    vector<pair<uint64_t, Json::Value>> transactions;

    for (auto &entry : filesystem::directory_iterator("mempool")) {
        ifstream txn_file(entry.path(), ifstream::binary);
        Json::Value txn;
        txn_file >> txn;

        // Check whether sum of inputs >= sum of outputs
        uint64_t sum_input = 0, sum_output = 0;
        for (auto &inp : txn["vin"])
            sum_input += inp["prevout"]["value"].asUInt64();
        for (auto &out : txn["vout"])
            sum_output += out["value"].asUInt64();
        if (sum_input < sum_output) {
            // Invalid Txn
            continue;
        }

        // Serialize the transaction
        string ser_txn = serialize_txn(txn);

        // Push the transaction to the vector
        // The first element of the pair is the fee to size ratio
        transactions.push_back({(sum_input - sum_output)/ser_txn.size(), txn});
    }

    // Sort the transactions in decreasing order of fee to size ratio
    sort(transactions.begin(), transactions.end(), greater<pair<uint64_t, Json::Value>>());

    vector<string> blockTxns;
    vector<string> blockTxnIds;
    vector<string> wTXIDs;
    int64_t reward = 625000000;
    uint32_t block_size = 80;

    for (auto &txn : transactions) {
        // Look for ony those transaction whose inputs are of type p2pkh, p2sh, v0_p2wpkh
        bool flag = false;
        for (auto &inp : txn.second["vin"]) {
            if (inp["prevout"]["scriptpubkey_type"] != "v0_p2wpkh" && inp["prevout"]["scriptpubkey_type"] != "p2pkh" && inp["prevout"]["scriptpubkey_type"] != "p2sh"){
                flag = true;
                break;
            }
        }
        if (flag) continue;

        string ser_txn = serialize_txn(txn.second);
        
        // Check block size limit and whether the transaction is valid
        if (block_size + ser_txn.size() < 1024*1024 && verify_txn(txn.second) >= 0) {
            blockTxns.push_back(ser_txn);
            wTXIDs.push_back(hash256(ser_wit(txn.second)));
            block_size += ser_txn.size();
            reward += txn.first;
        }
    }

    // Calculate TXIDs of all the transactions to be included in the block
    for (string txn : blockTxns)
        blockTxnIds.push_back(hash256(txn));

    // Calculate wTXID commitment
    wTXIDs.push_back(string(32, 0));
    string wTXID_commitment = hash256(getMerkleRoot(wTXIDs) + string(32, 0));
    string coinbaseTxn = genCoinbaseTxn(reward, wTXID_commitment);
    block_size += coinbaseTxn.size();

    // Calculate TXID of the coinbase transaction
    string coinbaseTxnId = "";
    char sha[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*) coinbaseTxn.c_str(), coinbaseTxn.length(), (unsigned char*) sha);
    SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        coinbaseTxnId.push_back(sha[i]);
    // Add the coinbase txn to the list of txns to be included in the block
    blockTxnIds.push_back(coinbaseTxnId);
    
    // Generate the block
    string block = "";
    // Generate the block header
    string blockHeader = genBlockHeader(blockTxnIds);
    block += blockHeader;
    string nonce = calcNonce(blockHeader);
    block += nonce;
    block = bstr2hexstr(block, block.length());
    block.push_back('\n');

    // Add the coinbase txn to the block
    block += bstr2hexstr(coinbaseTxn, coinbaseTxn.length());
    block.push_back('\n');

    // Add all the TXIDs to the block in reverse order because coinbase txn is at the end
    for (int i = blockTxnIds.size() - 1; i >= 0; i--) {
        reverse(blockTxnIds[i].begin(), blockTxnIds[i].end());
        block += bstr2hexstr(blockTxnIds[i], blockTxnIds[i].length());
        block.push_back('\n');
    }

    // Write the block to output.txt
    ofstream block_file("output.txt");
    block_file << block;
    block_file.close();

    // Just some messages
    cout << "Block mined successfully!" << endl;
    cout << "Nonce = " << bstr2hexstr(nonce, nonce.length()) << endl;
    cout << "Reward = " << reward << endl;
    cout << "Block size = " << double(block_size)/1024/1024 << " MB" << endl;
}


int main() {
    mine();

    return 0;
}