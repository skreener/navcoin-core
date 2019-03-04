// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libzerocoin/Accumulator.h"
#include "libzerocoin/Coin.h"
#include "primitives/transaction.h"
#include "serialize.h"
#include "uint256.h"

#ifndef ZEROMINT_H
#define ZEROMINT_H

class PublicMintChainData
{
public:
    PublicMintChainData() : outPoint(), blockHash(0) {}
    PublicMintChainData(COutPoint outPointIn, uint256 blockHashIn) : outPoint(outPointIn), blockHash(blockHashIn) {}

    uint256 GetBlockHash() const {
        return blockHash;
    }

    uint256 GetTxHash() const {
        return outPoint.hash;
    }

    int GetOutput() const {
        return outPoint.n;
    }

    COutPoint GetOutpoint() const {
        return outPoint;
    }

    bool IsNull() const {
        return outPoint.IsNull() && blockHash == 0;
    }

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(outPoint);
        READWRITE(blockHash);
    }

private:
    COutPoint outPoint;
    uint256 blockHash;
};

class WitnessData
{
public:
    WitnessData(const libzerocoin::ZerocoinParams* paramsIn) :
                accumulator(&paramsIn->accumulatorParams), accumulatorWitness(paramsIn) {}

    WitnessData(const libzerocoin::ZerocoinParams* paramsIn, libzerocoin::PublicCoin pubCoinIn,
                libzerocoin::Accumulator accumulatorIn, uint256 blockAccumulatorHashIn) :
                accumulator(paramsIn, accumulatorIn.getValue()),
                accumulatorWitness(paramsIn, accumulatorIn, pubCoinIn),
                blockAccumulatorHash(blockAccumulatorHashIn), nCount(0) {}

    WitnessData(libzerocoin::Accumulator accumulatorIn,
                libzerocoin::AccumulatorWitness accumulatorWitnessIn, uint256 blockAccumulatorHashIn) :
                accumulator(accumulatorIn), accumulatorWitness(accumulatorWitnessIn),
                blockAccumulatorHash(blockAccumulatorHashIn), nCount(0) {}

    WitnessData(const libzerocoin::ZerocoinParams* paramsIn, libzerocoin::Accumulator accumulatorIn,
                libzerocoin::AccumulatorWitness accumulatorWitnessIn, uint256 blockAccumulatorHashIn,
                int nCountIn) : accumulator(accumulatorIn), accumulatorWitness(accumulatorWitnessIn),
                blockAccumulatorHash(blockAccumulatorHashIn), nCount(nCountIn) {}

    void SetBlockAccumulatorHash(uint256 blockAccumulatorHashIn) {
        blockAccumulatorHash = blockAccumulatorHashIn;
    }

    uint256 GetBlockAccumulatorHash() const {
        return blockAccumulatorHash;
    }

    libzerocoin::Accumulator GetAccumulator() const {
        return accumulator;
    }

    libzerocoin::AccumulatorWitness GetAccumulatorWitness() const {
        return accumulatorWitness;
    }

    void Accumulate(CBigNum coinValue) {
        accumulator.increment(coinValue);
        accumulatorWitness.AddElement(coinValue);
        nCount++;
    }

    int GetCount() const {
        return nCount;
    }

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(accumulator);
        READWRITE(accumulatorWitness);
        READWRITE(blockAccumulatorHash);
        READWRITE(nCount);
    }

private:
    libzerocoin::Accumulator accumulator;
    libzerocoin::AccumulatorWitness accumulatorWitness;
    uint256 blockAccumulatorHash;
    int nCount;

};


class PublicMintWitnessData
{
public:
    template <typename Stream>
    PublicMintWitnessData(const libzerocoin::ZerocoinParams* p, Stream& strm) :
                          params(p), pubCoin(p), currentData(p), prevData(p), initialData(p)
    {
        strm >> *this;
    }

    PublicMintWitnessData(const libzerocoin::ZerocoinParams* paramsIn, const libzerocoin::PublicCoin pubCoinIn,
                          const PublicMintChainData chainDataIn, libzerocoin::Accumulator accumulatorIn,
                          uint256 blockAccumulatorHashIn) :
                          params(paramsIn), pubCoin(pubCoinIn), chainData(chainDataIn),
                          currentData(paramsIn, pubCoinIn, accumulatorIn, blockAccumulatorHashIn),
                          prevData(paramsIn, pubCoinIn, accumulatorIn, blockAccumulatorHashIn),
                          initialData(paramsIn, pubCoinIn, accumulatorIn, blockAccumulatorHashIn) {}

    PublicMintWitnessData(const PublicMintWitnessData& witness) : params(witness.params),
                          pubCoin(witness.pubCoin), chainData(witness.chainData), currentData(witness.currentData),
                          prevData(witness.prevData), initialData(witness.initialData) { }

    void Accumulate(CBigNum coinValue) {
        currentData.Accumulate(coinValue);
    }

    void SetBlockAccumulatorHash(uint256 blockAccumulatorHashIn) {
        currentData.SetBlockAccumulatorHash(blockAccumulatorHashIn);
    }

    void Backup() const {
        WitnessData copy(params, currentData.GetAccumulator(), currentData.GetAccumulatorWitness(),
                         currentData.GetBlockAccumulatorHash(), currentData.GetCount());
        prevData = copy;
    }

    void Recover() const {
        WitnessData copy(params, prevData.GetAccumulator(), prevData.GetAccumulatorWitness(),
                         prevData.GetBlockAccumulatorHash(), prevData.GetCount());
        currentData = copy;
    }

    bool Verify() const {
        return currentData.GetAccumulatorWitness().VerifyWitness(currentData.GetAccumulator(), pubCoin);
    }

    void Reset() const {
        WitnessData copy(params, initialData.GetAccumulator(), initialData.GetAccumulatorWitness(),
                         initialData.GetBlockAccumulatorHash(), initialData.GetCount());
        currentData = copy;
        prevData = copy;
    }

    uint256 GetBlockAccumulatorHash() const {
        return currentData.GetBlockAccumulatorHash();
    }

    uint256 GetPrevBlockAccumulatorHash() const {
        return prevData.GetBlockAccumulatorHash();
    }

    libzerocoin::Accumulator GetAccumulator() const {
        return currentData.GetAccumulator();
    }

    libzerocoin::AccumulatorWitness GetAccumulatorWitness() const {
        return currentData.GetAccumulatorWitness();
    }

    libzerocoin::PublicCoin GetPublicCoin() const {
        return pubCoin;
    }

    PublicMintChainData GetChainData() const {
        return chainData;
    }

    int GetCount() const {
        return currentData.GetCount();
    }

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(pubCoin);
        READWRITE(chainData);
        READWRITE(currentData);
        READWRITE(prevData);
        READWRITE(initialData);
    }

private:
    const libzerocoin::ZerocoinParams* params;
    libzerocoin::PublicCoin pubCoin;
    PublicMintChainData chainData;
    mutable WitnessData currentData;
    mutable WitnessData prevData;
    WitnessData initialData;

};

#endif // ZEROMINT_H
