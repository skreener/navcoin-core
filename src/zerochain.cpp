// Copyright (c) 2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zerochain.h"

using namespace libzerocoin;

bool TxOutToPublicCoin(const ZerocoinParams *params, const CTxOut& txout, PublicCoin& pubCoin, CValidationState* state)
{
    if (!txout.scriptPubKey.IsZerocoinMint())
        return false;

    CoinDenomination denomination = AmountToZerocoinDenomination(txout.nValue);
    if (denomination == ZQ_ERROR)
        return state != NULL ? state->DoS(100, error("TxOutToPublicCoin(): txout.nValue is not a valid denomination value")) : false;

    std::vector<unsigned char> c; CPubKey p;
    if(!txout.scriptPubKey.ExtractZerocoinMintData(p, c))
        return state != NULL ? state->DoS(100, error("TxOutToPublicCoin(): could not read mint data from txout.scriptPubKey")) : false;

    PublicCoin checkPubCoin(params, denomination, CBigNum(c), p);
    pubCoin = checkPubCoin;

    return true;
}

bool BlockToZerocoinMints(const ZerocoinParams *params, const CBlock* block, std::vector<PublicCoin> &vPubCoins)
{
    for (auto& tx : block->vtx) {
        for (auto& out : tx.vout) {
            if (!out.IsZerocoinMint())
                continue;

            PublicCoin pubCoin(params);

            CoinDenomination denomination = AmountToZerocoinDenomination(out.nValue);
            if (denomination == ZQ_ERROR)
                return error("BlockToZerocoinMints(): txout.nValue is not a valid denomination value");

            std::vector<unsigned char> c; CPubKey p;
            if(!out.scriptPubKey.ExtractZerocoinMintData(p, c))
                return error("BlockToZerocoinMints(): Could not extract Zerocoin mint data");

            PublicCoin checkPubCoin(params, denomination, CBigNum(c), p);
            vPubCoins.push_back(checkPubCoin);
        }
    }

    return true;
}

bool CheckZerocoinMint(const ZerocoinParams *params, const CTxOut& txout, const CCoinsViewCache& view, CValidationState& state, std::vector<std::pair<CBigNum, uint256>> vSeen, PublicCoin* pPubCoin)
{
    PublicCoin pubCoin(params);
    if(!TxOutToPublicCoin(params, txout, pubCoin, &state))
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
    int nHeight;

    if (pblocktree->ReadCoinMint(pubCoin.getValue(), txid) && IsTransactionInChain(txid, view, nHeight))
        return error("%s: pubcoin %s was already accumulated in tx %s from block %d", __func__,
                     pubCoin.getValue().GetHex().substr(0, 10),
                     txid.GetHex(), nHeight);

    return true;
}

bool TxInToCoinSpend(const ZerocoinParams *params, const CTxIn& txin, CoinSpend& coinSpend, CValidationState* state)
{
    return ScriptToCoinSpend(params, txin.scriptSig, coinSpend, state);
}

bool ScriptToCoinSpend(const ZerocoinParams *params, const CScript& scriptSig, CoinSpend& coinSpend, CValidationState* state)
{
    if (!scriptSig.IsZerocoinSpend())
        return false;

    std::vector<char, zero_after_free_allocator<char> > dataTxIn;
    dataTxIn.insert(dataTxIn.end(), scriptSig.begin() + BIGNUM_SIZE, scriptSig.end());
    CDataStream serializedCoinSpend(dataTxIn, SER_NETWORK, PROTOCOL_VERSION);

    libzerocoin::CoinSpend spend(params, serializedCoinSpend);

    coinSpend = spend;

    return true;
}

bool CheckZerocoinSpend(const ZerocoinParams *params, const CTxIn& txin, const CCoinsViewCache& view, CValidationState& state, std::vector<std::pair<CBigNum, uint256>> vSeen, CoinSpend* pCoinSpend)
{
    CoinSpend coinSpend(params);

    if(!TxInToCoinSpend(params, txin, coinSpend, &state))
        return state.DoS(100, error("CheckZerocoinSpend() : TxInToCoinSpend() failed"));

    if (pCoinSpend)
        *pCoinSpend = coinSpend;

    uint256 accumulatorChecksum = coinSpend.getAccumulatorChecksum();
    AccumulatorMap accumulatorMap(params);

    if (!accumulatorMap.Load(accumulatorChecksum))
        return state.DoS(100, error(strprintf("CheckZerocoinSpend() : Wrong accumulator checksum %s", accumulatorChecksum.ToString())));

    Accumulator accumulator(params, coinSpend.getDenomination());
    accumulator.setValue(accumulatorMap.GetValue(coinSpend.getDenomination()));

    if (!coinSpend.Verify(accumulator))
        return state.DoS(100, error("CheckZerocoinSpend() : CoinSpend does not verify"));

    uint256 txHash;
    int nHeight;

    if (pblocktree->ReadCoinSpend(coinSpend.getCoinSerialNumber(), txHash) && IsTransactionInChain(txHash, view, nHeight))
        return state.DoS(100, error(strprintf("CheckZerocoinSpend() : Serial Number %s is already spent in tx %s in block %d", coinSpend.getCoinSerialNumber().ToString(16), txHash.ToString(), nHeight)));

    for(auto& it : vSeen)
    {
        if (it.first == coinSpend.getCoinSerialNumber())
            return error("%s: serial number %s was already seen in this block", __func__,
                         coinSpend.getCoinSerialNumber().GetHex().substr(0, 10));
    }

    return true;

}

bool CountMintsFromHeight(unsigned int nInitialHeight, CoinDenomination denom, unsigned int& nRet)
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
                if(out.IsZerocoinMint() && denom == AmountToZerocoinDenomination(out.nValue))
                {
                    nRet++;
                }
            }
        }
        pindex = chainActive.Next(pindex);
    }

    return true;
}

bool CalculateWitnessForMint(const CTxOut& txout, const libzerocoin::PublicCoin& pubCoin, Accumulator& accumulator, AccumulatorWitness& accumulatorWitness, uint256& accumulatorChecksum, std::string& strError)
{
    if (!txout.IsZerocoinMint()) {
        strError = "Transaction output script is not a zerocoin mint.";
        return false;
    }

    uint256 blockHash;

    if (!pblocktree->ReadAccMint(pubCoin.getValue(), blockHash)) {
        strError = strprintf("Could not read mint with value %s from the db", pubCoin.getValue().GetHex());
        return false;
    }

    if (!mapBlockIndex.count(blockHash)) {
        strError = strprintf("Could not find block hash %s", blockHash.ToString());
        return false;
    }

    CBlockIndex* pindex = mapBlockIndex[blockHash];

    pindex = chainActive[pindex->nHeight - 1];

    if(!pindex) {
        strError = strprintf("Could not move back to a block index previous to the coin mint");
        return false;
    }

    AccumulatorMap accumulatorMap(&Params().GetConsensus().Zerocoin_Params);

    if(pindex->nAccumulatorChecksum != uint256() && !accumulatorMap.Load(pindex->nAccumulatorChecksum)) {
        strError = strprintf("Could not load Accumulators data from checksum %s at height %d",
                             pindex->nAccumulatorChecksum.GetHex(), pindex->nHeight);
        return false;
    }

    accumulator.setValue(accumulatorMap.GetValue(pubCoin.getDenomination()));
    accumulatorWitness.resetValue(accumulator, pubCoin);

    if (chainActive.Next(pindex)) {
        pindex = chainActive.Next(pindex);
        while (pindex) {
            CBlock block;

            if (!ReadBlockFromDisk(block, pindex, Params().GetConsensus())) {
                strError = strprintf("Could not read block %s from disk", pindex->GetBlockHash().ToString());
                return false;
            }

            if ((block.nVersion & VERSIONBITS_TOP_BITS_ZEROCOIN) != VERSIONBITS_TOP_BITS_ZEROCOIN) {
                strError = strprintf("Block %s is not a zerocoin block", pindex->GetBlockHash().ToString());
                return false;
            }

            for (auto& tx : block.vtx) {
                for (auto& out : tx.vout) {
                    if (!out.IsZerocoinMint())
                        continue;

                    PublicCoin pubCoinOut(&Params().GetConsensus().Zerocoin_Params);

                    if (!TxOutToPublicCoin(&Params().GetConsensus().Zerocoin_Params, out, pubCoinOut)) {
                        strError = strprintf("Could not extract Zerocoin mint data");
                        return false;
                    }

                    if (pubCoinOut.getDenomination() != pubCoin.getDenomination())
                        continue;

                    accumulatorWitness.AddElement(pubCoinOut);
                    accumulator.accumulate(pubCoinOut);

                    if (!accumulatorMap.Load(pindex->nAccumulatorChecksum)) {
                        return false;
                    }
                }
            }

            if(!chainActive.Next(pindex))
                break;

            pindex = chainActive.Next(pindex);
        }
    }

    if (!pindex) {
        strError = strprintf("Last block index is null");
        return false;
    }

    accumulatorChecksum = pindex->nAccumulatorChecksum;

    if (!accumulatorMap.Load(accumulatorChecksum)) {
        strError = strprintf("Could not load Accumulators data from checksum %s of last block index",
                             accumulatorChecksum.GetHex());
        return false;
    }

    accumulator.setValue(accumulatorMap.GetValue(pubCoin.getDenomination()));

    if (!accumulatorWitness.VerifyWitness(accumulator, pubCoin)) {
        strError = "Witness did not verify";
        return false;
    }

    return true;
}
