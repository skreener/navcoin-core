// Copyright (c) 2017-2018 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAV_ACCUMULATORMAP_H
#define NAV_ACCUMULATORMAP_H

#include "primitives/block.h"
#include "libzerocoin/Accumulator.h"
#include "libzerocoin/Coin.h"

class CBlock;
class CBlockIndex;
class CChain;

//A map with an accumulator for each denomination
class AccumulatorMap
{
private:
    const libzerocoin::ZerocoinParams* params;
    std::map<libzerocoin::CoinDenomination, std::unique_ptr<libzerocoin::Accumulator> > mapAccumulators;
    std::vector<uint256> vBlockHashes;
public:
    AccumulatorMap(const libzerocoin::ZerocoinParams* params);
    bool Accumulate(const libzerocoin::PublicCoin& pubCoin, bool fSkipValidation = false);
    bool Increment(const libzerocoin::CoinDenomination denom, const CBigNum& bnValue);
    CBigNum GetValue(libzerocoin::CoinDenomination denom);
    bool Get(libzerocoin::CoinDenomination denom, libzerocoin::Accumulator& accumulator);
    uint256 GetChecksum();
    uint256 GetBlockHash() { return vBlockHashes.size() > 0 ? vBlockHashes[vBlockHashes.size()-1] : uint256(); }
    uint256 GetFirstBlockHash() { return vBlockHashes.size() > 0 ? vBlockHashes[0] : uint256(); }
    void Reset();
    void Reset(const libzerocoin::ZerocoinParams* params2);
    bool Load(uint256 nCheckpoint);
    bool Save(uint256 blockHashIn = 0);
    bool Disconnect(uint256 blockHashIn);
};

bool CalculateAccumulatorChecksum(const CBlock* block, AccumulatorMap& accumulatorMap, std::vector<std::pair<CBigNum, uint256>>& vPubCoins);
#endif //NAV_ACCUMULATORMAP_H
