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

#ifndef SERIALNUMBERPROOFOFKNOWLEDGE_H
#define SERIALNUMBERPROOFOFKNOWLEDGE_H

#include "hash.h"
#include "bignum.h"
#include "Params.h"

using namespace std;

namespace libzerocoin {

class SerialNumberProofOfKnowledge
{
public:
    SerialNumberProofOfKnowledge(const ZerocoinParams* p);
    SerialNumberProofOfKnowledge(const ZerocoinParams* p, const CBigNum serialNumber);

    bool Verify(const CBigNum& coinSerialNumberPubKey) const;
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(t);
        READWRITE(r);
    }
private:
    const ZerocoinParams* params;
    CBigNum t;
    CBigNum r;
};
}

#endif // SERIALNUMBERPROOFOFKNOWLEDGE_H
