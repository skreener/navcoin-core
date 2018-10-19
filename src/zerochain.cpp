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

bool BlockToZeroCoinMints(const libzerocoin::ZerocoinParams *params, const CBlock* block, std::vector<libzerocoin::PublicCoin> &vPubCoins)
{
    for (auto& tx : block->vtx) {
        for (auto& out : tx.vout) {
            if (!out.IsZerocoinMint())
                continue;

            libzerocoin::PublicCoin pubCoin(params);

            libzerocoin::CoinDenomination denomination = libzerocoin::AmountToZerocoinDenomination(out.nValue);
            if (denomination == libzerocoin::ZQ_ERROR)
                return error("BlockToZeroCoinMints(): txout.nValue is not a valid denomination value");

            std::vector<unsigned char> c; CPubKey p;
            if(!out.scriptPubKey.ExtractZerocoinMintData(p, c))
                return error("BlockToZeroCoinMints(): Could not extract Zerocoin mint data");

            libzerocoin::PublicCoin checkPubCoin(params, denomination, CBigNum(c), p);
            vPubCoins.push_back(checkPubCoin);
        }
    }

    return true;
}

bool CheckZerocoinMint(const libzerocoin::ZerocoinParams *params, const CTxOut& txout, CValidationState& state, std::vector<std::pair<CBigNum, uint256>> vSeen, libzerocoin::PublicCoin* pPubCoin)
{
    libzerocoin::PublicCoin pubCoin(params);
    if(!TxOutToPublicCoin(params, txout, pubCoin, state))
        return state.DoS(100, error("CheckZerocoinMint(): TxOutToPublicCoin() failed"));

    if (pPubCoin)
        *pPubCoin = pubCoin;

    if (!pubCoin.isValid())
        return state.DoS(100, error("CheckZerocoinMint() : PubCoin does not validate"));

    for(auto& it : vSeen)
    {
        if (it.first == pubCoin.getValue())
            return error("%s: pubcoin %s was already seen in this block", __func__,
                         pubCoin.getValue().GetHex().substr(0, 10));
    }

    uint256 txid;
    if (pblocktree->ReadCoinMint(pubCoin.getValue(), txid))
        return error("%s: pubcoin %s was already accumulated in tx %s", __func__,
                     pubCoin.getValue().GetHex().substr(0, 10),
                     txid.GetHex());

    return true;
}

bool CountMintsFromHeight(unsigned int nInitialHeight, libzerocoin::CoinDenomination denom, unsigned int& nRet)
{
    nRet = 0;
    CBlockIndex* pindex = chainActive[nInitialHeight];

    while(pindex)
    {
        CBlock block;
        if (!ReadBlockFromDisk(block, pindex, Params().GetConsensus()))
            return false;

        for (auto& tx: block.vtx)
        {
            for (CTxOut& out: tx.vout)
            {
                if(out.IsZerocoinMint() && denom == libzerocoin::AmountToZerocoinDenomination(out.nValue))
                {
                    nRet++;
                }
            }
        }
        pindex = chainActive.Next(pindex);
    }

    return true;
}
