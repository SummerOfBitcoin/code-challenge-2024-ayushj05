#include <iostream>
#include <jsoncpp/json/json.h>
#include <fstream>
#include <filesystem>
#include <set>
#include <sstream>
#include "script.h"
using namespace std;

set<string> inputs_used;

void mine() {
    vector<Json::Value> transactions;       // Vector to store all transactions

    for (auto &entry : filesystem::directory_iterator("mempool")) {
        ifstream txn_file(entry.path(), ifstream::binary);
        Json::Value txn;
        txn_file >> txn;
        transactions.push_back(txn);
    }

    set<string> s;
    
    for (auto &txn : transactions) {
        for (auto &inp : txn["vin"]) {
            if (inp["prevout"]["scriptpubkey_type"] == "p2sh"){
                vector<string> ops = getOps(inp["inner_redeemscript_asm"].asString());
                if (identifyScriptType(ops) == "Invalid")
                    cout << inp["inner_redeemscript_asm"].asString() << '\n' << endl;
                s.insert(identifyScriptType(ops));
            }
        }
    }

    for (auto &i : s) {
        cout << i << endl;
    }
}


int main() {
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
    //         if (inp["prevout"]["scriptpubkey_type"] != "p2pkh"){
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



    // ifstream txn_file("mempool/ff907975dc0cfa299e908e5fba6df56c764866d9a9c22828824c28b8e4511320.json");         // p2pkh
    // ifstream txn_file("mempool/598ed237ba816ab2c80ba7b57cd48c3ad87a15bf33736061eb0304ee23412d85.json");
    // ifstream txn_file("mempool/fff53b0fda0ab690ddaa23c84536e0d364a736bb93137a76ebf5d78f57cdd32f.json");         // p2wpkh
    // ifstream txn_file("mempool/ff7f94f1344696386e4e75e33114fd158055104793eb151e73f0b032a073b35e.json");         // p2wsh
    ifstream txn_file("mempool/598ed237ba816ab2c80ba7b57cd48c3ad87a15bf33736061eb0304ee23412d85.json");         // p2sh-p2wpkh
    

    std::ostringstream tmp;
    tmp << txn_file.rdbuf();
    std::string txn_str = tmp.str();

    Json::Value txn;
    Json::Reader reader;
    reader.parse(txn_str.c_str(), txn);
    
    // string ser_txn = serialize_txn(txn, true);
    // string ser_txn_hex = bstr2hexstr(ser_txn, ser_txn.length());
    // cout << ser_txn_hex << '\n' << endl;
    
    // char sha[SHA256_DIGEST_LENGTH];
    // SHA256((unsigned char*) ser_txn.c_str(), ser_txn.length(), (unsigned char*) sha);
    // SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);
    // for (int i = 0; i < SHA256_DIGEST_LENGTH/2; i++)
    //     swap(sha[i], sha[SHA256_DIGEST_LENGTH - i - 1]);
    
    // SHA256((unsigned char*) sha, SHA256_DIGEST_LENGTH, (unsigned char*) sha);
    // cout << "Triple SHA256: " << bstr2hexstr(sha, SHA256_DIGEST_LENGTH) << endl;

    cout << txn << endl;
    cout << verify_txn(txn) << endl;


    return 0;
}