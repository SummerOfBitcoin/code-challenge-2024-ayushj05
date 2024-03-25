#include <iostream>
#include <jsoncpp/json/json.h>
#include <fstream>
#include <filesystem>
#include <set>
#include <sstream>
#include "script.h"
using namespace std;

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
    //     if (check) cout << verify_txn(txn, txn_str) << '\n' << endl;
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

    // // Test for int2hex function defined in serialize.h
    // cout << int2hex(uint32_t(UINT32_MAX - 255 + 97)) << endl;   // Should print a in the beginning



    ifstream txn_file("mempool/ff907975dc0cfa299e908e5fba6df56c764866d9a9c22828824c28b8e4511320.json");

    std::ostringstream tmp;
    tmp << txn_file.rdbuf();
    std::string txn_str = tmp.str();

    Json::Value txn;
    Json::Reader reader;
    reader.parse(txn_str.c_str(), txn);

    cout << verify_txn(txn, txn_str) << endl;





    return 0;
}