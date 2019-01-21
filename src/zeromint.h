// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libzerocoin/Coin.h"
#include "serialize.h"
#include "uint256.h"

#ifndef ZEROMINT_H
#define ZEROMINT_H

class PublicMintChainData
{
public:
    PublicMintChainData() : txHash(0), blockHash(0) {}
    PublicMintChainData(uint256 txHashIn, uint256 blockHashIn) : txHash(txHashIn), blockHash(blockHashIn) {}

    uint256 GetBlockHash() const {
        return blockHash;
    }

    uint256 GetTxHash() const {
        return txHash;
    }

    bool IsNull() const {
        return txHash == 0 && blockHash == 0;
    }

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(txHash);
        READWRITE(blockHash);
    }

private:
    uint256 txHash;
    uint256 blockHash;
};

#endif // ZEROMINT_H
