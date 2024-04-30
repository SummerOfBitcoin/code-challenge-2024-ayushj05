# Design Approach

Overall, the assignment has the following code files:
* `main.cpp`: Contains the mine() function that is executed in the main function.
* `block.h`: Contains all the helper functions for creating a block (generating merkle root, creating coinbase txn, etc).
* `script.h`: Contins all the functions related to script identification and verification, signature verification, etc.
* `serialize.h`: Contains all the functions related to transaction serialization (as required in `script.h`), along with functions for conversion to bitcoin's datatypes.

The block mining procedure is performed by the `mine()` routine:
1. First, all those transactions for which the sum of input values is greater than the sum of output values are pushed into the `transactions` vector.
2. Next, the transactions are sorted in descending order with respect to the ratio of "txn fees" to "size taken up by the txn in the block" (i.e., size of serialized transaction).
3. Since I couldn't complete the validation of p2tr, p2wsh and p2ms transactions, the `mine()` function only validates p2pkh, p2wpkh, and p2sh (excluding p2sh-p2ms and p2sh-p2wsh) transactions.
4. Next, the transactions are put into the `blockTxns` vector, while ensuring that the block size doesn't exceed the 1 MB limitation (may contain some rough calculation), and most importantly, the transaction is valid (checked using `verify_txn()`).
5. Next, the "wTXID commitment" is calculated to use in generating the coinbase txn. Then the coinbase txn is generated and its txid pushed in the `blockTxnIds` vector.
6. Using this `blockTxnIds` vector, the block header is generated (excluding the nonce) and pushed into the `block` string (string in hex format). Next, the nonce is calculated (using `calcNonce()` function in `block.h`) for PoW. and appended to the `block`. The block header part of our block is complete.
7. Now, the serialized coinbase txn is appended to the `block`, followed by all the txids in `blockTxnIds` in reverse order (as the coinbase txn is at last; To maintain this order, I have everywhere performed operations in the reversed order of `blockTxnIds`, wherever the order is important, such as calculating merkle root). This completes our `block` string. We then write this string to `output.txt`.

Wait, but what about double spends?

* For them, we maintain an `inputs_used` set, which stores the txids of inputs used.
* This is done in the `verify_txn()` function in `script.h`. Also, a `txn_inputs` set is maintained. This cointains the inputs of the current txn that have been verified till now.
* If the transaction is valid, as per our mining logic described above, we are anyways putting it in the block. So just freeze the inputs.
* Now, for every txn, the `verify_txn()` first checks whether the inputs aren't included in the `inputs_used` set as well as in the `txn_inputs` set. If present, return `false` (i.e., txn is invalid). Else, insert input into `txn_inputs` set.

# Implementation Details

The `mine()` routine can be summarized using the following pseudo code:
```
mine () {
    vector<uint64_t, Json::Value> txns;

    for txn in mempool:
        // Valid txn
        if sum_inps >= sum_ops:
            ser_txn = serialize_txn(txn);
            txns.push({(sum_inps - sum_ops)/ser_txn.size(), txn});
    
    sort_descending(txns);

    reward = 625000000

    for txn in txns:
        if all vins of type "v0_p2wpkh" or "p2pkh" or "p2sh":
            if txn is valid and doesn't overflow block:
                insert into blockTxns to include txn in the block
                reward += txn_fees
    
    wTXID_commitment = calculate from wTXIDs of txns in blockTxns
    coinbaseTxn = calculate using reward and wTXID_commitment
    insert into blockTxns

    blockHeader = genBlockHeader(TXIDs of txns in blockTxns);
    nonce = calcNonce(blockHeader);

    block += blockHeader + nonce + "\n" + coinbaseTxn + "\n";
    for txn in reverse(blockTxns):
        block += txid_of_txn + "\n";
    
    write block to output.txt
}
```

Now the next main thing is `verify_txn(txn)`:

```
verify_txn (Json::Value &txn) {
    if sum_inputs < sum_outputs:
        return -1;
    
    txn_inputs = <empty>
    for inp in txn:
        if inputs_used.found(inp) or txn_inputs.found(inp)
            return -1;
        
        Depending upon scriptPubKeyType:
            verify the script;
            if invalid:
                return -1;
    
    if all inputs valid:
        for inp in txn:
            inputs_used.insert(txid_of_inp);
        
        return txn_fees;
}
```

# Results and Performance

I got a score of 73/100 for this mining program.

The program has been designed while taking care of minimizing string parsing, as much as possible. For example, especially in the `serialize.h` file, all the functions related to txn serialization and data type conversions have been optimized so that the task to be performed is done in just a single iteration of the string.

Why is it important?\
These functions are some common functions and will be called a lot of times, thus essentially amounting to executing these functions on the entire list of transactions (which is VERY BIG). Thus, reducing the time complexity, even from O(2N) to O(N), can give us significant improvement in the runtime.

# Conclusion

## Insights Gained
This project gave me a great and detailed idea of how each type of transaction script is verified, as per the BIPs. Even though I couldn't complete the implementation of verification of all script types, I went through the corresponding BIPs, and also got to know the importance of various script types and why they were proposed.

## Future Improvement
This mining program only mines transactions that are of the type p2pkh, p2wpkh, and p2sh (excluding p2sh-p2ms and p2sh-p2wsh). So, one possible improvement on this assignment is implementing the verification of p2tr, p2wsh and p2ms (and hence, p2sh-p2ms and p2sh-p2wsh as well) transactions.

Also, I haven't done any sanity checks in this assignment. This needs to be implemented if in case the transaction has contradictory fields (Example: `scriptpubkey_type` doesn't match with `scriptpubkey_asm`).

Further, there could be some room for improvement by removing redundant transaction parsings and implementing some faster mining method.

## Resources

* https://learnmeabitcoin.com/
* https://github.com/bitcoin/bips
* https://developer.bitcoin.org/reference/transactions.html
* https://en.bitcoin.it/wiki/Script
