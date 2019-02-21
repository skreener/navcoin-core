/**
* @file       Math.h
*
* @brief      Bulletproofs Rangeproof
*             Inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
*
* @author     alex v
* @date       December 2018
*
* @copyright  Copyright 2018 alex v
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2018-2019 The NavCoin Core developers

#ifndef BULLETPROOF_RANGEPROOF_H
#define BULLETPROOF_RANGEPROOF_H

#include "Bulletproofs.h"
#include "Params.h"
#include "Math.h"

#include "hash.h"

#include <cmath>

class BulletproofRangeproof
{
public:
    static const size_t maxN = 64;
    static const size_t maxM = 16;

    BulletproofRangeproof(const libzerocoin::IntegerGroupParams* in_p) : params(in_p), bulletproof(in_p) {}

    void Prove(std::vector<CBigNum> v, std::vector<CBigNum> gamma);
    bool Verify();

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(V);
        READWRITE(L);
        READWRITE(R);
        READWRITE(a);
        READWRITE(S);
        READWRITE(T1);
        READWRITE(T2);
        READWRITE(taux);
        READWRITE(mu);
        READWRITE(a);
        READWRITE(b);
        READWRITE(t);
    }

    std::string ToString() const {
        return "V: " + libzerocoin::toStringVector(V) + "\nL: "+ libzerocoin::toStringVector(L) + "\nR: " + libzerocoin::toStringVector(R)
                + "\nA: " + A.ToString(16).substr(0,8)  + "\nS: " + S.ToString(16).substr(0,8)
                 + "\nT1: " + T1.ToString(16).substr(0,8) + "\nT2: " + T2.ToString(16).substr(0,8) + "\ntaux: " + taux.ToString(16).substr(0,8) + "\nmu: " + mu.ToString(16).substr(0,8)
                 + "\na: " + a.ToString(16).substr(0,8) + "\nb: " + b.ToString(16).substr(0,8) + "\nt: " + t.ToString(16).substr(0,8);
    }

    std::vector<CBigNum> V;
    std::vector<CBigNum> L;
    std::vector<CBigNum> R;
    CBigNum A;
    CBigNum S;
    CBigNum T1;
    CBigNum T2;
    CBigNum taux;
    CBigNum mu;
    CBigNum a;
    CBigNum b;
    CBigNum t;

private:
    const libzerocoin::IntegerGroupParams* params;
    const libzerocoin::Bulletproofs bulletproof;
};

bool VerifyBulletproof(const libzerocoin::IntegerGroupParams* p, const std::vector<BulletproofRangeproof>& proof);

#endif // BULLETPROOF_RANGEPROOF_H
