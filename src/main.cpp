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
    vector<pair<uint64_t, Json::Value>> transactions;       // Vector to store all transactions

    for (auto &entry : filesystem::directory_iterator("mempool")) {
        ifstream txn_file(entry.path(), ifstream::binary);
        Json::Value txn;
        txn_file >> txn;

        uint64_t sum_input = 0, sum_output = 0;
        for (auto &inp : txn["vin"])
            sum_input += inp["prevout"]["value"].asUInt64();
        for (auto &out : txn["vout"])
            sum_output += out["value"].asUInt64();
        if (sum_input < sum_output) {
            // Invalid Txn
            continue;
        }

        string ser_txn = serialize_txn(txn);

        transactions.push_back({(sum_input - sum_output)/ser_txn.size(), txn});
    }

    sort(transactions.begin(), transactions.end(), greater<pair<uint64_t, Json::Value>>());

    vector<string> blockTxns;
    vector<string> blockTxnIds;
    int64_t reward = 625000000;
    uint32_t block_size = 80;

    for (auto &txn : transactions) {
        bool flag = false;
        for (auto &inp : txn.second["vin"]) {
            if (inp["prevout"]["scriptpubkey_type"] != "p2pkh"){
                flag = true;
                break;
            }
        }
        if (flag) continue;

        string ser_txn = serialize_txn(txn.second);
        
        if (block_size + ser_txn.size() < 1024*1024 && verify_txn(txn.second) >= 0) {
            blockTxns.push_back(ser_txn);
            block_size += ser_txn.size();
            reward += txn.first;
        }
    }

    // Calculate TXIDs of all the transactions to be included in the block
    for (string txn : blockTxns) {
        char sha[SHA256_DIGEST_LENGTH];
        SHA256((unsigned char*) txn.c_str(), txn.length(), (unsigned char*) sha);
        SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);
        string txid = "";
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
            txid.push_back(sha[i]);
        blockTxnIds.push_back(txid);
    }

    vector<string> wTXIDs = blockTxnIds;
    wTXIDs.push_back(string(32, 0));
    string wTXID_commitment = hash256(getMerkleRoot(wTXIDs) + string(32, 0));
    string coinbaseTxn = genCoinbaseTxn(reward, wTXID_commitment);
    block_size += coinbaseTxn.size();

    string coinbaseTxnId = "";
    char sha[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*) coinbaseTxn.c_str(), coinbaseTxn.length(), (unsigned char*) sha);
    SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        coinbaseTxnId.push_back(sha[i]);
    blockTxnIds.push_back(coinbaseTxnId);
    
    string block = "";
    string blockHeader = genBlockHeader(blockTxnIds);
    block += blockHeader;
    string nonce = calcNonce(blockHeader);
    block += nonce;
    block = bstr2hexstr(block, block.length());
    block.push_back('\n');

    block += bstr2hexstr(coinbaseTxn, coinbaseTxn.length());
    block.push_back('\n');

    // Add all the transactions to the block in reverse order because coinbase txn is at the end
    for (int i = blockTxnIds.size() - 1; i >= 0; i--) {
        reverse(blockTxnIds[i].begin(), blockTxnIds[i].end());
        block += bstr2hexstr(blockTxnIds[i], blockTxnIds[i].length());
        block.push_back('\n');
    }

    ofstream block_file("output.txt");
    block_file << block;
    block_file.close();

    cout << "Block mined successfully!" << endl;
    cout << "Nonce = " << bstr2hexstr(nonce, nonce.length()) << endl;
    cout << "Reward = " << reward << endl;
    cout << "Block size = " << double(block_size)/1024/1024 << " MB" << endl;
}


int main() {
    mine();

    // vector<Json::Value> transactions;       // Vector to store all transactions

    // // Iterate through all files in the mempool directory and read the transactions
    // for (auto &entry : filesystem::directory_iterator("mempool")) {
    //     ifstream txn_file(entry.path(), ifstream::binary);

    //     std::ostringstream tmp;
    //     tmp << txn_file.rdbuf();
    //     std::string txn_str = tmp.str();

    //     Json::Value txn;
    //     Json::Reader reader;
    //     reader.parse(txn_str.c_str(), txn);

    //     bool check = true;
    //     for (auto &inp : txn["vin"]) {
    //         if (inp["prevout"]["scriptpubkey_type"] != "p2pkh"){
    //             check = false;
    //             break;
    //         }
    //     }
    //     if (check) cout << verify_txn(txn, txn_str) << endl;
    //     // transactions.push_back(txn);
    // }


    // vector<Json::Value> transactions;       // Vector to store all transactions
    // for (auto &entry : filesystem::directory_iterator("mempool")) {
    //     ifstream txn_file(entry.path(), ifstream::binary);
    //     Json::Value txn;
    //     txn_file >> txn;
    //     transactions.push_back(txn);
    // }
    // set<string> s;
    // for (auto &txn : transactions) {
    //     for (auto &inp : txn["vin"]) {
    //         if (inp["prevout"]["scriptpubkey_type"] == "p2sh"){
    //             vector<string> ops = getOps(inp["inner_redeemscript_asm"].asString());
    //             if (identifyScriptType(ops) == "Invalid")
    //                 cout << inp["inner_redeemscript_asm"].asString() << '\n' << endl;
    //             s.insert(identifyScriptType(ops));
    //         }
    //     }
    // }

    // for (auto &i : s) {
    //     cout << i << endl;
    // }


    // // Test for int2hex function defined in serialize.h
    // cout << int2hex(uint32_t(UINT32_MAX - 255 + 97)) << endl;   // Should print a in the beginning



    // ifstream txn_file("mempool/ff907975dc0cfa299e908e5fba6df56c764866d9a9c22828824c28b8e4511320.json");
    // // ifstream txn_file("mempool/fff53b0fda0ab690ddaa23c84536e0d364a736bb93137a76ebf5d78f57cdd32f.json");

    // std::ostringstream tmp;
    // tmp << txn_file.rdbuf();
    // std::string txn_str = tmp.str();

    // Json::Value txn;
    // Json::Reader reader;
    // reader.parse(txn_str.c_str(), txn);
    
    // string ser_txn = sertialize_txn(txn);
    // string ser_txn_hex = bstr2hexstr(ser_txn, ser_txn.length());
    
    // char sha[SHA256_DIGEST_LENGTH];
    // SHA256((unsigned char*) ser_txn.c_str(), ser_txn.length(), (unsigned char*) sha);
    // SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);
    // SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);
    // // cout << "SHA256: " << bstr2hexstr(sha, SHA256_DIGEST_LENGTH) << endl;

    // cout << verify_txn(txn) << endl;


    // u_int64_t total_fees = 0;
    
    // for (auto &entry : filesystem::directory_iterator("mempool")) {
    //     ifstream txn_file(entry.path(), ifstream::binary);
    //     Json::Value txn;
    //     txn_file >> txn;
        
    //     bool flag = false;
    //     for (auto &inp : txn["vin"]) {
    //         if (inp["prevout"]["scriptpubkey_type"] != "p2pkh" && inp["prevout"]["scriptpubkey_type"] != "v0_p2wpkh"){
    //             flag = true;
    //             break;
    //         }
    //     }
    //     if (flag) continue;

    //     total++;

    //     int64_t status = verify_txn(txn);
    //     if (status >= 0) {
    //         valid_txns++;
    //         total_fees += status;
    //     }
    //     else
    //         invalid_txns++;
    // }
    // cout << total << ' ' << valid_txns << ' ' << invalid_txns << endl;

    // // for (auto &i : s) {
    // //     cout << i << endl;
    // // }
    // cout << total_fees << endl;



    // // ifstream txn_file("mempool/ff907975dc0cfa299e908e5fba6df56c764866d9a9c22828824c28b8e4511320.json");         // p2pkh
    // // ifstream txn_file("mempool/fff53b0fda0ab690ddaa23c84536e0d364a736bb93137a76ebf5d78f57cdd32f.json");         // p2wpkh
    // ifstream txn_file("mempool/ff7f94f1344696386e4e75e33114fd158055104793eb151e73f0b032a073b35e.json");         // p2wsh
    // // ifstream txn_file("mempool/598ed237ba816ab2c80ba7b57cd48c3ad87a15bf33736061eb0304ee23412d85.json");         // p2sh-p2wpkh
    // // ifstream txn_file("mempool/598ed237ba816ab2c80ba7b57cd48c3ad87a15bf33736061eb0304ee23412d85.json");
    

    // std::ostringstream tmp;
    // tmp << txn_file.rdbuf();
    // std::string txn_str = tmp.str();

    // Json::Value txn;
    // Json::Reader reader;
    // reader.parse(txn_str.c_str(), txn);
    
    // // string ser_txn = serialize_txn(txn, true);
    // // string ser_txn_hex = bstr2hexstr(ser_txn, ser_txn.length());
    // // cout << ser_txn_hex << '\n' << endl;
    
    // // char sha[SHA256_DIGEST_LENGTH];
    // // SHA256((unsigned char*) ser_txn.c_str(), ser_txn.length(), (unsigned char*) sha);
    // // SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);
    // // for (int i = 0; i < SHA256_DIGEST_LENGTH/2; i++)
    // //     swap(sha[i], sha[SHA256_DIGEST_LENGTH - i - 1]);
    
    // // SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);
    // // cout << "Triple SHA256: " << bstr2hexstr(sha, SHA256_DIGEST_LENGTH) << endl;

    // cout << txn << endl;
    // cout << verify_txn(txn) << endl;


    return 0;
}