// Copyright (c) 2017-2018 The PIVX developers
// Copyright (c) 2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "zeroaccumulators.h"
#include "main.h"
#include "txdb.h"
#include "libzerocoin/Denominations.h"

using namespace libzerocoin;
using namespace std;

uint32_t GetChecksumFromBn(const CBigNum &bnValue)
{
    CDataStream ss(SER_GETHASH, 0);
    ss << bnValue;
    uint256 hash = Hash(ss.begin(), ss.end());

    return hash.Get32();
}

//Construct accumulators for all denominations
AccumulatorMap::AccumulatorMap(const libzerocoin::ZerocoinParams* params)
{
    this->params = params;
    for (auto& denom : zerocoinDenomList) {
        unique_ptr<Accumulator> uptr(new Accumulator(params, denom));
        mapAccumulators.insert(make_pair(denom, std::move(uptr)));
    }
}

//Reset each accumulator to its default state
void AccumulatorMap::Reset()
{
    Reset(params);
}

void AccumulatorMap::Reset(const libzerocoin::ZerocoinParams* params2)
{
    this->params = params2;
    mapAccumulators.clear();
    for (auto& denom : zerocoinDenomList) {
        unique_ptr<Accumulator> uptr(new Accumulator(params, denom));
        mapAccumulators.insert(make_pair(denom, std::move(uptr)));
    }
}

//Add a zerocoin to the accumulator of its denomination.
bool AccumulatorMap::Accumulate(const PublicCoin& pubCoin, bool fSkipValidation)
{
    CoinDenomination denom = pubCoin.getDenomination();
    if (denom == CoinDenomination::ZQ_ERROR)
        return false;

    if (fSkipValidation)
        mapAccumulators.at(denom)->increment(pubCoin.getValue());
    else
        mapAccumulators.at(denom)->accumulate(pubCoin);

    return true;
}

//Add a zerocoin to the accumulator of its denomination using directly its value
bool AccumulatorMap::Increment(const CoinDenomination denom, const CBigNum& bnValue)
{
    mapAccumulators.at(denom)->increment(bnValue);
    return true;
}

//Get the value of a specific accumulator
CBigNum AccumulatorMap::GetValue(CoinDenomination denom)
{
    if (denom == CoinDenomination::ZQ_ERROR)
        return CBigNum(0);
    return mapAccumulators.at(denom)->getValue();
}

//Calculate a 32bit checksum of each accumulator value. Concatenate checksums into uint256
uint256 AccumulatorMap::GetChecksum()
{
    uint256 nCombinedChecksum;

    //Prevent possible overflows from future changes to the list and forgetting to update this code
    assert(zerocoinDenomList.size() == 8);
    for (auto& denom : zerocoinDenomList) {
        if(mapAccumulators.at(denom)) {
            CBigNum bnValue = mapAccumulators.at(denom)->getValue();
            uint32_t nCheckSum = GetChecksumFromBn(bnValue);
            nCombinedChecksum = nCombinedChecksum << 32 | nCheckSum;
        }
    }

    return nCombinedChecksum;
}

//Load a checkpoint containing 8 32bit checksums of accumulator values.
bool AccumulatorMap::Load(uint256 nChecksum)
{
    std::vector<std::pair<libzerocoin::CoinDenomination,CBigNum>> toRead;
    if (!pblocktree->ReadZeroCoinAccumulator(nChecksum, toRead))
        return error("%s : cannot read zerocoin accumulator checksum %s", __func__, nChecksum.ToString());

    for(auto& it : toRead)
    {
        mapAccumulators.at(it.first)->setValue(it.second);
    }

    return true;
}

bool AccumulatorMap::Save()
{
    std::vector<std::pair<libzerocoin::CoinDenomination,CBigNum>> vAcc;

    for(auto& it : mapAccumulators)
    {
        vAcc.push_back(std::make_pair(it.first, it.second->getValue()));
    }

    if (!pblocktree->WriteZeroCoinAccumulator(GetChecksum(), vAcc))
        return error("%s : cannot write zerocoin accumulator checksum %s", __func__, GetChecksum().ToString());

    return true;
}

bool CalculateAccumulatorChecksum(CChain& chain, int nHeight, AccumulatorMap& mapAccumulators)
{
    std::pair<int, uint256> firstZero = make_pair(0, uint256());

    pblocktree->ReadFirstZeroCoinBlock(firstZero);

    CBlockIndex* pLastChecksum = chain[max((unsigned int)firstZero.first,
                 nHeight - Params().GetConsensus().nRecalculateAccumulatorChecksum)];
    CBlockIndex* pLastAccumulated = chain[max((unsigned int)firstZero.first,
                 nHeight - Params().GetConsensus().nAccumulatorChecksumBlockDelay)];

    if (pLastChecksum)
    {
        if (pLastChecksum->nAccumulatorChecksum != uint256())
            mapAccumulators.Load(pLastChecksum->nAccumulatorChecksum);

        if (pLastAccumulated)
        {
            if ((nHeight % Params().GetConsensus().nRecalculateAccumulatorChecksum) == 0)
            {
                unsigned int nCount = 0;

                while (pLastAccumulated && nCount < Params().GetConsensus().nRecalculateAccumulatorChecksum)
                {
                    if ((pLastAccumulated->nVersion & VERSIONBITS_TOP_BITS_ZEROCOIN) != VERSIONBITS_TOP_BITS_ZEROCOIN ||
                            pLastAccumulated->nHeight < firstZero.first)
                        break;

                    for (auto& it: pLastChecksum->mapMints)
                        for (auto& pc: it.second)
                            if (!mapAccumulators.Increment(it.first, pc))
                                return error("Error! Trying to accumulate coin\n");

                    pLastAccumulated = pLastAccumulated->pprev;

                    nCount++;
                }

            }

        }
    }

    return true;
}
