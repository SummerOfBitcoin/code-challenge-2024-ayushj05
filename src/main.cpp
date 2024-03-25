#include <iostream>
#include <jsoncpp/json/json.h>
#include <fstream>
#include <filesystem>
#include <set>
#include "script.h"
using namespace std;

int main() {
    // vector<Json::Value> transactions;       // Vector to store all transactions

    // // Iterate through all files in the mempool directory and read the transactions
    // for (auto &entry : filesystem::directory_iterator("mempool")) {
    //     ifstream txn_file(entry.path(), ifstream::binary);
    //     Json::Value txn;
    //     txn_file >> txn;
    //     bool check = true;
    //     for (auto &inp : txn["vin"]) {
    //         if (inp["prevout"]["scriptpubkey_type"] != "p2pkh"){
    //             check = false;
    //             break;
    //         }
    //     }
    //     if (check) cout << verify_txn(txn) << '\n' << endl;
    //     // transactions.push_back(txn);
    // }

    // set<string> s;
    // for (auto &txn : transactions) {
    //     for (auto &inp : txn["vin"]) {
    //         if (inp["prevout"]["scriptpubkey_type"] == "p2pkh"){
    //             // cout << inp["prevout"]["scriptpubkey_asm"].asString() << endl;
    //             vector<string> ops = getOps(inp["prevout"]["scriptpubkey_asm"].asString());
    //             // cout << ops[0] << " " << ops[1] << " " << ops[2] << " " << ops[4] << " " << ops[5] << endl;
    //             s.insert(ops[0] + " " + ops[1] + " " + ops[2] + " " + ops[4] + " " + ops[5]);
    //             // s.insert(ops.size());
    //         }
    //     }
    // }

    // for (auto &i : s) {
    //     cout << i << endl;
    // }

    ifstream txn_file("mempool/ff907975dc0cfa299e908e5fba6df56c764866d9a9c22828824c28b8e4511320.json", ifstream::binary);
    Json::Value txn;
    txn_file >> txn;
    cout << verify_txn(txn) << endl;

    // char sha[65];
    // sha256(txn.toStyledString().c_str(), sha);
    // cout << sha << endl;

    

    return 0;
}