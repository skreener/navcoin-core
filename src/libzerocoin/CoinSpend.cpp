/**
 * @file       CoinSpend.cpp
 *
 * @brief      CoinSpend class for the Zerocoin library.
 *
 * @author     Ian Miers, Christina Garman and Matthew Green
 * @date       June 2013
 *
 * @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
 * @license    This project is released under the MIT license.
 **/
// Copyright (c) 2017-2018 The PIVX developers
// Copyright (c) 2018 The NavCoin Core developers

#include "CoinSpend.h"
#include <iostream>
#include <sstream>

namespace libzerocoin
{
CoinSpend::CoinSpend(const ZerocoinParams* paramsCoin, const ZerocoinParams* paramsAcc, const PrivateCoin& coin, Accumulator& a, const uint32_t& checksum,
                     const AccumulatorWitness& witness, const uint256& ptxHash, const SpendType& spendType) : accChecksum(checksum),
    ptxHash(ptxHash),
    accumulatorPoK(&paramsAcc->accumulatorParams),
    serialNumberSoK(paramsCoin),
    serialNumberPoK(paramsCoin),
    commitmentPoK(&paramsCoin->serialNumberSoKCommitmentGroup,
                  &paramsAcc->accumulatorParams.accumulatorPoKCommitmentGroup),
    spendType(spendType)
{
    denomination = coin.getPublicCoin().getDenomination();
    coinSerialNumber = paramsCoin->coinCommitmentGroup.g.pow_mod(coin.getSerialNumber(), paramsCoin->serialNumberSoKCommitmentGroup.groupOrder);
    version = coin.getVersion();
    if (!static_cast<int>(version)) //todo: figure out why version does not make it here
        version = 1;

    // Sanity check: let's verify that the Witness is valid with respect to
    // the coin and Accumulator provided.
    if (!(witness.VerifyWitness(a, coin.getPublicCoin()))) {
        throw std::runtime_error("Accumulator witness does not verify");
    }

    // 1: Generate two separate commitments to the public coin (C), each under
    // a different set of public parameters. We do this because the RSA accumulator
    // has specific requirements for the commitment parameters that are not
    // compatible with the group we use for the serial number proof.
    // Specifically, our serial number proof requires the order of the commitment group
    // to be the same as the modulus of the upper group. The Accumulator proof requires a
    // group with a significantly larger order.
    const Commitment fullCommitmentToCoinUnderSerialParams(&paramsCoin->serialNumberSoKCommitmentGroup, coin.getPublicCoin().getValue());
    this->serialCommitmentToCoinValue = fullCommitmentToCoinUnderSerialParams.getCommitmentValue();

    const Commitment fullCommitmentToCoinUnderAccParams(&paramsAcc->accumulatorParams.accumulatorPoKCommitmentGroup, coin.getPublicCoin().getValue());
    this->accCommitmentToCoinValue = fullCommitmentToCoinUnderAccParams.getCommitmentValue();

    // 2. Generate a ZK proof that the two commitments contain the same public coin.
    this->commitmentPoK = CommitmentProofOfKnowledge(&paramsCoin->serialNumberSoKCommitmentGroup, &paramsAcc->accumulatorParams.accumulatorPoKCommitmentGroup, fullCommitmentToCoinUnderSerialParams, fullCommitmentToCoinUnderAccParams);

    // Now generate the two core ZK proofs:
    // 3. Proves that the committed public coin is in the Accumulator (PoK of "witness")
    this->accumulatorPoK = AccumulatorProofOfKnowledge(&paramsAcc->accumulatorParams, fullCommitmentToCoinUnderAccParams, witness, a);

    // 4. Proves that the coin is correct w.r.t. serial number and hidden coin secret
    // (This proof is bound to the coin 'metadata', i.e., transaction hash)
    uint256 hashSig = signatureHash();
    this->serialNumberSoK = SerialNumberSignatureOfKnowledge(paramsCoin, coin, fullCommitmentToCoinUnderSerialParams, hashSig);

    //5. Zero knowledge proof of the serial number
    this->serialNumberPoK = SerialNumberProofOfKnowledge(paramsCoin, coin.getSerialNumber());
}

bool CoinSpend::Verify(const Accumulator& a) const
{

    if (a.getDenomination() != this->denomination) {
        throw std::runtime_error("CoinsSpend::Verify: failed, denominations do not match");
        return false;
    }

    // Verify both of the sub-proofs using the given meta-data
    if (!commitmentPoK.Verify(serialCommitmentToCoinValue, accCommitmentToCoinValue)) {
        throw std::runtime_error("CoinsSpend::Verify: commitmentPoK failed");
        return false;
    }

    if (!accumulatorPoK.Verify(a, accCommitmentToCoinValue)) {
        throw std::runtime_error("CoinsSpend::Verify: accumulatorPoK failed");
        return false;
    }

    if (!serialNumberSoK.Verify(coinSerialNumber, serialCommitmentToCoinValue, signatureHash())) {
        throw std::runtime_error("CoinsSpend::Verify: serialNumberSoK failed.");
        return false;
    }

    if (!serialNumberPoK.Verify(coinSerialNumber)) {
        throw std::runtime_error("CoinsSpend::Verify: serialNumberPoK failed.");
        return false;
    }

    return true;
}

const uint256 CoinSpend::signatureHash() const
{
    CHashWriter h(0, 0);
    h << serialCommitmentToCoinValue << accCommitmentToCoinValue << commitmentPoK << accumulatorPoK << ptxHash
      << coinSerialNumber << accChecksum << denomination << spendType;

    return h.GetHash();
}

std::string CoinSpend::ToString() const
{
    std::stringstream ss;
    ss << "CoinSpend:\n version=" << (int)version << " signatureHash=" << signatureHash().GetHex() << " spendtype=" << spendType << "\n";
    return ss.str();
}

bool CoinSpend::HasValidSerial(ZerocoinParams* params) const
{
    return IsValidSerial(params, coinSerialNumber);
}

CBigNum CoinSpend::CalculateValidSerial(ZerocoinParams* params)
{
    CBigNum bnSerial = coinSerialNumber;
    bnSerial = bnSerial.mul_mod(CBigNum(1),params->coinCommitmentGroup.groupOrder);
    return bnSerial;
}

} /* namespace libzerocoin */
