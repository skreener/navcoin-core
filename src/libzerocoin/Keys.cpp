/**
* @file       Keys.cpp
*
* @brief      Keys class for the Zerocoin library.
*
* @author     alex v
* @date       September 2018
*
* @copyright  Copyright 2018 alex v
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2018 The NavCoin Core developers

#include "Keys.h"

namespace libzerocoin {
void GenerateParameters(const ZerocoinParams* params, CBigNum& oj, CBigNum& ok, CBigNum& bc) {
    oj = CBigNum::randBignum(params->coinCommitmentGroup.groupOrder);
    ok = CBigNum::randBignum(params->coinCommitmentGroup.groupOrder);
    bc = params->coinCommitmentGroup.g.pow_mod(oj, params->coinCommitmentGroup.modulus).mul_mod(
         params->coinCommitmentGroup.h.pow_mod(ok, params->coinCommitmentGroup.modulus), params->coinCommitmentGroup.modulus);
}

bool CPrivateSpendKey::GetObfuscationJ(CBigNum& obfuscationJ) const {
    if (this->params->initialized == false) return false;
    obfuscationJ = CBigNum(oj);
    return true;
}
bool CPrivateSpendKey::GetObfuscationK(CBigNum& obfuscationK) const {
    if (this->params->initialized == false) return false;
    obfuscationK = CBigNum(ok);
    return true;
}
bool CPrivateSpendKey::GetPrivKey(CPrivKey& zerokey) const {
    if (this->params->initialized == false) return false;
    zerokey = zpk;
    return true;
}

bool CPrivateViewKey::GetBlindingCommitment(CBigNum& blindingCommitment) const {
    if (this->params->initialized == false) return false;
    blindingCommitment = CBigNum(bc);
    return true;
}
bool CPrivateViewKey::GetPrivKey(CPrivKey& zerokey) const {
    if (this->params->initialized == false) return false;
    zerokey = zpk;
    return true;
}

bool CPrivateAddress::GetBlindingCommitment(CBigNum& blindingCommitment) const {
    if (this->params->initialized == false) return false;
    blindingCommitment = CBigNum(bc);
    return true;
}
bool CPrivateAddress::GetPubKey(CPubKey& zerokey) const {
    if (this->params->initialized == false) return false;
    zerokey = zpk;
    return true;
}
bool CPrivateAddress::MintPublicCoin(CoinDenomination d, PublicCoin& pubcoin) const {
    if (this->params->initialized == false) return false;
    pubcoin = PublicCoin(this->params, d, zpk, CBigNum(bc));
    return true;
}
}
