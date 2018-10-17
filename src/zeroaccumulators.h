// Copyright (c) 2017-2018 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAV_ACCUMULATORMAP_H
#define NAV_ACCUMULATORMAP_H

#include "libzerocoin/Accumulator.h"
#include "libzerocoin/Coin.h"

//A map with an accumulator for each denomination
class AccumulatorMap
{
private:
    const libzerocoin::ZerocoinParams* params;
    std::map<libzerocoin::CoinDenomination, std::unique_ptr<libzerocoin::Accumulator> > mapAccumulators;
public:
    AccumulatorMap(const libzerocoin::ZerocoinParams* params);
    bool Accumulate(const libzerocoin::PublicCoin& pubCoin, bool fSkipValidation = false);
    bool Increment(const libzerocoin::CoinDenomination denom, const CBigNum& bnValue);
    CBigNum GetValue(libzerocoin::CoinDenomination denom);
    uint256 GetChecksum();
    void Reset();
    void Reset(const libzerocoin::ZerocoinParams* params2);
    bool Load(uint256 nCheckpoint);
    bool Save();
};
#endif //NAV_ACCUMULATORMAP_H
