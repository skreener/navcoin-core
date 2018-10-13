/**
* @file       SerialNumberProofOfKnowledge.cpp
*
* @brief      SerialNumberProofOfKnowledge class for the Zerocoin library.
*
* @author     alex v
* @date       September 2018
*
* @copyright  Copyright 2018 alex v
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2018 The NavCoin Core developers

#include "SerialNumberProofOfKnowledge.h"
#include <iostream>
#include <sstream>

namespace libzerocoin {

SerialNumberProofOfKnowledge::SerialNumberProofOfKnowledge(const ZerocoinParams* p): params(p) { }

SerialNumberProofOfKnowledge::SerialNumberProofOfKnowledge(const ZerocoinParams* p, const CBigNum serialNumber, const uint256 signatureHash) : params(p)
{
    CBigNum y = params->coinCommitmentGroup.g.pow_mod(serialNumber, params->serialNumberSoKCommitmentGroup.groupOrder);
    CBigNum v = CBigNum::randBignum(params->serialNumberSoKCommitmentGroup.groupOrder);
    CBigNum t = params->coinCommitmentGroup.g.pow_mod(v, params->serialNumberSoKCommitmentGroup.groupOrder);
    CHashWriter hasher(0,0);
    hasher << *params << y << t << signatureHash;
    CBigNum c = CBigNum(hasher.GetHash());
    CBigNum r = v - (c * serialNumber);
    this->t = t;
    this->r = r;
}
bool SerialNumberProofOfKnowledge::Verify(const CBigNum& coinSerialNumberPubKey, const uint256 signatureHash) const
{
    CHashWriter hasher(0,0);
    hasher << *params << coinSerialNumberPubKey << t << signatureHash;
    CBigNum c = CBigNum(hasher.GetHash());
    CBigNum u = params->coinCommitmentGroup.g.pow_mod(r, params->serialNumberSoKCommitmentGroup.groupOrder).mul_mod(coinSerialNumberPubKey.pow_mod(c, params->serialNumberSoKCommitmentGroup.groupOrder), params->serialNumberSoKCommitmentGroup.groupOrder);
    return (t == u);
}
}
