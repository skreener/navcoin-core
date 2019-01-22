// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libzerocoin/Accumulator.h"
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
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(txHash);
        READWRITE(blockHash);
    }

private:
    uint256 txHash;
    uint256 blockHash;
};

class WitnessData
{
public:
    WitnessData(const libzerocoin::ZerocoinParams* paramsIn) :
                accumulator(&paramsIn->accumulatorParams), accumulatorWitness(paramsIn) {}

    WitnessData(const libzerocoin::ZerocoinParams* paramsIn, libzerocoin::PublicCoin pubCoinIn,
                libzerocoin::Accumulator accumulatorIn, uint256 accumulatorChecksumIn) :
                accumulator(paramsIn, pubCoinIn.getDenomination(), accumulatorIn.getValue()),
                accumulatorWitness(paramsIn, accumulatorIn, pubCoinIn),
                accumulatorChecksum(accumulatorChecksumIn), nCount(0) {}

    WitnessData(libzerocoin::Accumulator accumulatorIn,
                libzerocoin::AccumulatorWitness accumulatorWitnessIn, uint256 accumulatorChecksumIn) :
                accumulator(accumulatorIn), accumulatorWitness(accumulatorWitnessIn),
                accumulatorChecksum(accumulatorChecksumIn), nCount(0) {}

    WitnessData(const libzerocoin::ZerocoinParams* paramsIn, libzerocoin::Accumulator accumulatorIn,
                libzerocoin::AccumulatorWitness accumulatorWitnessIn, uint256 accumulatorChecksumIn,
                int nCountIn) : accumulator(accumulatorIn), accumulatorWitness(accumulatorWitnessIn),
                accumulatorChecksum(accumulatorChecksumIn), nCount(nCountIn) {}

    WitnessData(const WitnessData& witness) : accumulator(witness.accumulator),
                accumulatorWitness(witness.accumulatorWitness), accumulatorChecksum(witness.accumulatorChecksum),
                nCount(witness.nCount) { }

    void SetChecksum(uint256 checksum) {
        accumulatorChecksum = checksum;
    }

    uint256 GetChecksum() const {
        return accumulatorChecksum;
    }

    libzerocoin::Accumulator GetAccumulator() const {
        return accumulator;
    }

    libzerocoin::AccumulatorWitness GetAccumulatorWitness() const {
        return accumulatorWitness;
    }

    void Accumulate(CBigNum coinValue) {
        accumulator.increment(coinValue);
        accumulatorWitness.addRawValue(coinValue);
        nCount++;
    }

    int GetCount() {
        return nCount;
    }

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(accumulator);
        READWRITE(accumulatorWitness);
        READWRITE(accumulatorChecksum);
        READWRITE(nCount);
    }

private:
    libzerocoin::Accumulator accumulator;
    libzerocoin::AccumulatorWitness accumulatorWitness;
    uint256 accumulatorChecksum;
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
                          uint256 checksumIn) :
                          params(paramsIn), pubCoin(pubCoinIn), chainData(chainDataIn),
                          currentData(paramsIn, pubCoinIn, accumulatorIn, checksumIn),
                          prevData(paramsIn, pubCoinIn, accumulatorIn, checksumIn),
                          initialData(paramsIn, pubCoinIn, accumulatorIn, checksumIn) {}

    PublicMintWitnessData(const PublicMintWitnessData& witness) : params(witness.params),
                          pubCoin(witness.pubCoin), chainData(witness.chainData), currentData(witness.currentData),
                          prevData(witness.prevData), initialData(witness.initialData) { }

    void Accumulate(CBigNum coinValue) {
        currentData.Accumulate(coinValue);
    }

    void SetChecksum(uint256 checksum) {
        currentData.SetChecksum(checksum);
    }

    void Backup() {
        WitnessData copy(params, currentData.GetAccumulator(), currentData.GetAccumulatorWitness(),
                         currentData.GetChecksum(), currentData.GetCount());
        prevData = copy;
    }

    void Reset() {
        WitnessData copy(params, initialData.GetAccumulator(), initialData.GetAccumulatorWitness(),
                         initialData.GetChecksum(), initialData.GetCount());
        currentData = copy;
        prevData = copy;
    }

    uint256 GetChecksum() const {
        return currentData.GetChecksum();
    }

    libzerocoin::Accumulator GetAccumulator() const {
        return currentData.GetAccumulator();
    }

    libzerocoin::AccumulatorWitness GetAccumulatorWitness() const {
        return currentData.GetAccumulatorWitness();
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
    WitnessData currentData;
    WitnessData prevData;
    WitnessData initialData;

};

#endif // ZEROMINT_H
