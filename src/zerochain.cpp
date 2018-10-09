// Copyright (c) 2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zerochain.h"

bool TxOutToPublicCoin(const libzerocoin::ZerocoinParams *params, const CTxOut& txout, libzerocoin::PublicCoin& pubCoin, CValidationState& state)
{
    if (!txout.scriptPubKey.IsZerocoinMint())
        return false;

    libzerocoin::CoinDenomination denomination = libzerocoin::AmountToZerocoinDenomination(txout.nValue);
    if (denomination == libzerocoin::ZQ_ERROR)
        return state.DoS(100, error("TxOutToPublicCoin(): txout.nValue is not a valid denomination value"));

    std::vector<unsigned char> c; CPubKey p;
    if(!txout.scriptPubKey.ExtractZerocoinMintData(p, c))
        return false;

    libzerocoin::PublicCoin checkPubCoin(params, denomination, CBigNum(c), p);
    pubCoin = checkPubCoin;

    return true;
}
