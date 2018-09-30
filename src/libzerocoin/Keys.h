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

#ifndef KEYS_H
#define KEYS_H

#include "Coin.h"
#include "key.h"
#include "Params.h"

#include <vector>

namespace libzerocoin {
class CPrivateAddress
{
public:
    CPrivateAddress(const ZerocoinParams* p) : params(p) { }
    CPrivateAddress(const ZerocoinParams* p, CBigNum blindingCommitment, CPubKey zeroKey) : params(p), bc(blindingCommitment.getvch()), zpk(zeroKey) {
        unsigned int vectorSize = (params->coinCommitmentGroup.modulus.bitSize()/8)+1;
        bc.resize(vectorSize);
    }
    CPrivateAddress(const ZerocoinParams* p, CBigNum blindingCommitment, CKey zeroKey) : params(p), bc(blindingCommitment.getvch()), zpk(zeroKey.GetPubKey()) {
        unsigned int vectorSize = (params->coinCommitmentGroup.modulus.bitSize()/8)+1;
        bc.resize(vectorSize);
    }

    bool GetBlindingCommitment(CBigNum& blindingCommitment) const;
    bool GetPubKey(CPubKey& zerokey) const;
    bool MintPublicCoin(CoinDenomination d, PublicCoin& pubcoin) const;

    const ZerocoinParams* GetParams() const { return params; }

    bool operator<(const CPrivateAddress& rhs) const {
        CBigNum lhsBN; CBigNum rhsBN;
        this->GetBlindingCommitment(lhsBN);
        rhs.GetBlindingCommitment(rhsBN);
        return lhsBN < rhsBN;
    }

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(bc);
        READWRITE(zpk);
    }
private:
    const ZerocoinParams* params;
    std::vector<unsigned char> bc;
    CPubKey zpk;
};
class CPrivateViewKey
{
public:
    CPrivateViewKey(const ZerocoinParams* p) : params(p) { }
    CPrivateViewKey(const ZerocoinParams* p, CBigNum blindingCommitment, CPrivKey zeroPrivKey) : params(p), bc(blindingCommitment.getvch()), zpk(zeroPrivKey) {
        unsigned int vectorSize = (params->coinCommitmentGroup.modulus.bitSize()/8)+1;
        bc.resize(vectorSize);
    }

    bool GetBlindingCommitment(CBigNum& blindingCommitment) const;
    bool GetPrivKey(CPrivKey& zerokey) const;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(bc);
        READWRITE(zpk);
    }
private:
    const ZerocoinParams* params;
    std::vector<unsigned char> bc;
    CPrivKey zpk;
};
class CPrivateSpendKey
{
public:
    CPrivateSpendKey(const ZerocoinParams* p) : params(p) { }
    CPrivateSpendKey(const ZerocoinParams* p, CBigNum obfuscationJ, CBigNum obfuscationK, CPrivKey zeroPrivKey) : params(p), oj(obfuscationJ.getvch()), ok(obfuscationK.getvch()), zpk(zeroPrivKey) {
        unsigned int vectorSize = (params->coinCommitmentGroup.groupOrder.bitSize()/8)+1;
        oj.resize(vectorSize);
        ok.resize(vectorSize);
    }

    bool GetObfuscationJ(CBigNum& obfuscationJ) const;
    bool GetObfuscationK(CBigNum& obfuscationK) const;
    bool GetPrivKey(CPrivKey& zerokey) const;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(oj);
        READWRITE(ok);
        READWRITE(zpk);
    }
private:
    const ZerocoinParams* params;
    std::vector<unsigned char> oj;
    std::vector<unsigned char> ok;
    CPrivKey zpk;
};
void GenerateParameters(const ZerocoinParams* params, CBigNum& oj, CBigNum& ok, CBigNum& bc, CKey& zk);
}

#endif // KEYS_H
