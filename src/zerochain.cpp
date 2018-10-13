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

bool CheckZerocoinMint(const libzerocoin::ZerocoinParams *params, const CTxOut& txout, CValidationState& state, libzerocoin::PublicCoin* pPubCoin)
{
    libzerocoin::PublicCoin pubCoin(params);
    if(!TxOutToPublicCoin(params, txout, pubCoin, state))
        return state.DoS(100, error("CheckZerocoinMint(): TxOutToPublicCoin() failed"));

    if (pPubCoin)
        *pPubCoin = pubCoin;

    if (!pubCoin.isValid())
        return state.DoS(100, error("CheckZerocoinMint() : PubCoin does not validate"));

    uint256 txid;
    if (pblocktree->ReadCoinMint(pubCoin.getValue(), txid))
        return error("%s: pubcoin %s was already accumulated in tx %s", __func__,
                     pubCoin.getValue().GetHex().substr(0, 10),
                     txid.GetHex());

    return true;
}
