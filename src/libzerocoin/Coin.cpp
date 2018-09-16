/**
 * @file       Coin.cpp
 *
 * @brief      PublicCoin and PrivateCoin classes for the Zerocoin library.
 *
 * @author     Ian Miers, Christina Garman and Matthew Green
 * @date       June 2013
 *
 * @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
 * @license    This project is released under the MIT license.
 **/
// Copyright (c) 2017-2018 The PIVX developers
// Copyright (c) 2018 The NavCoin Core developers

#include <stdexcept>
#include <iostream>
#include "Coin.h"
#include "Commitment.h"
#include "pubkey.h"
#include "utilstrencodings.h"

namespace libzerocoin {

PublicCoin::PublicCoin(const ZerocoinParams* p):
    params(p), value(0), denomination(ZQ_ERROR) {
    if (this->params->initialized == false) {
        throw std::runtime_error("Params are not initialized");
    }
};

PublicCoin::PublicCoin(const ZerocoinParams* p, const CoinDenomination d, const CBigNum value, const CPubKey pubKey) : params(p), value(value), denomination(d), pubKey(pubKey) {
    if (this->params->initialized == false) {
        throw std::runtime_error("Params are not initialized");
    }

    std::vector<CoinDenomination>::const_iterator it;

    it = find (zerocoinDenomList.begin(), zerocoinDenomList.end(), denomination);
    if(it == zerocoinDenomList.end()){
        throw std::runtime_error("Denomination does not exist");
    }

    if(!isValid())
        throw std::runtime_error("Commitment Value of Public Coin is invalid");
}

PublicCoin::PublicCoin(const ZerocoinParams* p, const CoinDenomination d, const CPubKey destPubKey, const CBigNum blindingCommitment): params(p), denomination(d) {
    // Verify that the parameters are valid
    if(this->params->initialized == false)
        throw std::runtime_error("Params are not initialized");

    std::vector<CoinDenomination>::const_iterator it;

    it = find (zerocoinDenomList.begin(), zerocoinDenomList.end(), denomination);
    if(it == zerocoinDenomList.end()){
        throw std::runtime_error("Denomination does not exist");
    }

    CKey key;
    bool fFound = false;

    // Repeat this process up to MAX_COINMINT_ATTEMPTS times until
    // we obtain a valid commitment value.
    for (uint32_t attempt = 0; attempt < MAX_COINMINT_ATTEMPTS; attempt++) {
        // Generate a new Key Pair
        key.MakeNewKey(false);

        CPrivKey shared_secret;
        if(!key.ECDHSecret(destPubKey, shared_secret))
            throw std::runtime_error("PrivateCoin::PrivateCoin(): Could not calculate ECDH Secret");

        uint256 pre_s(Hash(shared_secret.begin(), shared_secret.end()));
        uint256 pre_r(Hash(pre_s.begin(), pre_s.end()));

        CBigNum s = CBigNum(pre_s) % (this->params->coinCommitmentGroup.groupOrder/2);
        CBigNum r = CBigNum(pre_r) % (this->params->coinCommitmentGroup.groupOrder/2);

        // Manually compute a Pedersen commitment to the serial number "s" under randomness "r" and obfuscate it with blinding commitment "b"
        // C = g^s * h^r * b mod p
        CBigNum commitmentValue = (this->params->coinCommitmentGroup.g.pow_mod(s, this->params->coinCommitmentGroup.modulus) * blindingCommitment).mul_mod(this->params->coinCommitmentGroup.h.pow_mod(r, this->params->coinCommitmentGroup.modulus), this->params->coinCommitmentGroup.modulus);

        // First verify that the commitment is a prime number
        // in the appropriate range. If not, we repeat the process.
        if (commitmentValue.isPrime(ZEROCOIN_MINT_PRIME_PARAM) &&
                commitmentValue >= params->accumulatorParams.minCoinValue &&
                commitmentValue <= params->accumulatorParams.maxCoinValue) {
            // Found a valid coin. Store it.
            this->denomination = denomination;
            this->pubKey = key.GetPubKey();
            this->value = commitmentValue;
            this->version = CURRENT_VERSION;

            // Success! We're done.
            fFound = true;
            break;
        }
    }

    // We only get here if we did not find a coin within
    // MAX_COINMINT_ATTEMPTS. Throw an exception.
    if(!fFound)
        throw std::runtime_error("Unable to mint a new Zerocoin (too many attempts)");
}


bool PublicCoin::isValid() const
{
    if (this->params->accumulatorParams.minCoinValue >= value) {
        throw std::runtime_error("PublicCoin::isValid(): value is too low");
    }

    if (value > this->params->accumulatorParams.maxCoinValue) {
        throw std::runtime_error("PublicCoin::isValid(): value is too high");
    }

    if (!value.isPrime(params->zkp_iterations)) {
        throw std::runtime_error("PublicCoin::isValid(): value is not prime");
    }

    return true;
}

PrivateCoin::PrivateCoin(const ZerocoinParams* p, const CoinDenomination denomination, const CKey privKey, const CPubKey mintPubKey,
                         const CBigNum obfuscation_j, const CBigNum obfuscation_k, const CBigNum commitment_value) : params(p), publicCoin(p), randomness(0), serialNumber(0), fValid(false)
{
    // Verify that the parameters are valid
    if(!this->params->initialized)
        throw std::runtime_error("PrivateCoin::PrivateCoin(): Params are not initialized");

    CPrivKey shared_secret;
    if(!privKey.ECDHSecret(mintPubKey, shared_secret))
        throw std::runtime_error("PrivateCoin::PrivateCoin(): Could not calculate ECDH Secret");

    uint256 pre_s(Hash(shared_secret.begin(), shared_secret.end()));
    uint256 pre_r(Hash(pre_s.begin(), pre_s.end()));

    CBigNum s = CBigNum(pre_s) % (this->params->coinCommitmentGroup.groupOrder/2);
    CBigNum r = CBigNum(pre_r) % (this->params->coinCommitmentGroup.groupOrder/2);

    // Manually compute a Pedersen commitment to the serial number "s" under randomness "r" after obfuscating them with "j" and "k"
    // C = g^(s+j) * h^(r+k) mod p
    CBigNum commitmentValue = this->params->coinCommitmentGroup.g.pow_mod(s + obfuscation_j, this->params->coinCommitmentGroup.modulus).mul_mod(this->params->coinCommitmentGroup.h.pow_mod(r + obfuscation_k, this->params->coinCommitmentGroup.modulus), this->params->coinCommitmentGroup.modulus);

    if (commitmentValue.isPrime(ZEROCOIN_MINT_PRIME_PARAM) &&
            commitmentValue >= params->accumulatorParams.minCoinValue &&
            commitmentValue <= params->accumulatorParams.maxCoinValue) {
        this->serialNumber = s + obfuscation_j;
        this->randomness = r + obfuscation_k;
        if(commitmentValue == commitment_value) {
            CBigNum blindingCommitment = this->params->coinCommitmentGroup.g.pow_mod(obfuscation_j, this->params->coinCommitmentGroup.modulus).mul_mod(this->params->coinCommitmentGroup.h.pow_mod(obfuscation_k, this->params->coinCommitmentGroup.modulus), this->params->coinCommitmentGroup.modulus);
            this->publicCoin = PublicCoin(p, denomination, commitmentValue, mintPubKey);
            fValid = true;
        }
    }

    this->version = CURRENT_VERSION;

}

bool PrivateCoin::isValid()
{
    if (!fValid)
        return false;

    if (!IsValidSerial(params, serialNumber)) {
        throw std::runtime_error("PrivateCoin::isValid(): Serial not valid");
        return false;
    }

    return getPublicCoin().isValid();
}

bool IsValidSerial(const ZerocoinParams* params, const CBigNum& bnSerial)
{
    return bnSerial > 0 && bnSerial < params->coinCommitmentGroup.groupOrder;
}

} /* namespace libzerocoin */
