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
void GenerateParameters(const ZerocoinParams* params, libzerocoin::ObfuscationValue& oj, libzerocoin::ObfuscationValue& ok, libzerocoin::BlindingCommitment& bc, CKey& zk) {
    CBigNum oj1 = CBigNum::randBignum(params->coinCommitmentGroup.groupOrder);
    CBigNum ok1 = CBigNum::randBignum(params->coinCommitmentGroup.groupOrder);
    CBigNum oj2 = CBigNum::randBignum(params->coinCommitmentGroup.groupOrder);
    CBigNum ok2 = CBigNum::randBignum(params->coinCommitmentGroup.groupOrder);
    CBigNum bc1 = params->coinCommitmentGroup.g.pow_mod(oj1, params->coinCommitmentGroup.modulus).mul_mod(
          params->coinCommitmentGroup.h.pow_mod(ok1, params->coinCommitmentGroup.modulus), params->coinCommitmentGroup.modulus);
    CBigNum bc2 = params->coinCommitmentGroup.g.pow_mod(oj2, params->coinCommitmentGroup.modulus).mul_mod(
          params->coinCommitmentGroup.h.pow_mod(ok2, params->coinCommitmentGroup.modulus), params->coinCommitmentGroup.modulus);
    oj = std::make_pair(oj1,oj2);
    ok = std::make_pair(ok1,ok2);
    bc = std::make_pair(bc1,bc2);
    zk.MakeNewKey(true);
}

bool CPrivateSpendKey::GetObfuscationJ(libzerocoin::ObfuscationValue& obfuscationJ) const {
    if (this->params->initialized == false) return false;
    obfuscationJ = std::make_pair(CBigNum(oj1),CBigNum(oj2));
    return true;
}
bool CPrivateSpendKey::GetObfuscationK(libzerocoin::ObfuscationValue& obfuscationK) const {
    if (this->params->initialized == false) return false;
    obfuscationK = std::make_pair(CBigNum(ok1),CBigNum(ok2));
    return true;
}
bool CPrivateSpendKey::GetPrivKey(CPrivKey& zerokey) const {
    if (this->params->initialized == false) return false;
    zerokey = zpk;
    return true;
}

bool CPrivateViewKey::GetBlindingCommitment(libzerocoin::BlindingCommitment& blindingCommitment) const {
    if (this->params->initialized == false) return false;
    blindingCommitment = std::make_pair(CBigNum(bc1),CBigNum(bc2));
    return true;
}
bool CPrivateViewKey::GetPrivKey(CPrivKey& zerokey) const {
    if (this->params->initialized == false) return false;
    zerokey = zpk;
    return true;
}

bool CPrivateAddress::GetBlindingCommitment(libzerocoin::ObfuscationValue& blindingCommitment) const {
    if (this->params->initialized == false) return false;
    blindingCommitment = std::make_pair(CBigNum(bc1),CBigNum(bc2));
    return true;
}
bool CPrivateAddress::GetPubKey(CPubKey& zerokey) const {
    if (this->params->initialized == false) return false;
    zerokey = zpk;
    return true;
}
}
