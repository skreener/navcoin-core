// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zerochain.h"
#include "zerotx.h"

using namespace libzerocoin;

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

            std::vector<unsigned char> c; CPubKey p; std::vector<unsigned char> i;
            if(!out.scriptPubKey.ExtractZerocoinMintData(p, c, i))
                return error("BlockToZerocoinMints(): Could not extract Zerocoin mint data");

            CBigNum pid;
            pid.setvch(i);

            PublicCoin checkPubCoin(params, denomination, CBigNum(c), p, pid);
            vPubCoins.push_back(checkPubCoin);
        }
    }

    return true;
}

bool CheckZerocoinMint(const ZerocoinParams *params, const CTxOut& txout, const CCoinsViewCache& view, CValidationState& state, std::vector<std::pair<CBigNum, PublicMintChainData>> vSeen, PublicCoin* pPubCoin)
{
    PublicCoin pubCoin(params);
    if(!TxOutToPublicCoin(params, txout, pubCoin, &state, true))
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

    PublicMintChainData zeroMint;
    int nHeight;

    if (pblocktree->ReadCoinMint(pubCoin.getValue(), zeroMint) && zeroMint.GetTxHash() != 0 && IsTransactionInChain(zeroMint.GetTxHash(), view, nHeight))
        return error("%s: pubcoin %s was already accumulated in tx %s from block %d", __func__,
                     pubCoin.getValue().GetHex().substr(0, 10),
                     zeroMint.GetTxHash().GetHex(), nHeight);

    return true;
}

bool CheckZerocoinSpend(const ZerocoinParams *params, const CTxIn& txin, const CCoinsViewCache& view, CValidationState& state, std::vector<std::pair<CBigNum, uint256>> vSeen, CoinSpend* pCoinSpend)
{
    CoinSpend coinSpend(params);

    if(!TxInToCoinSpend(params, txin, coinSpend))
        return state.DoS(100, error("CheckZerocoinSpend() : TxInToCoinSpend() failed"));

    if (pCoinSpend)
        *pCoinSpend = coinSpend;

    uint256 accumulatorChecksum = coinSpend.getAccumulatorChecksum();
    AccumulatorMap accumulatorMap(params);

    if (!accumulatorMap.Load(accumulatorChecksum))
        return state.DoS(100, error(strprintf("CheckZerocoinSpend() : Wrong accumulator checksum %s", accumulatorChecksum.ToString())));

    Accumulator accumulator(params, coinSpend.getDenomination());
    accumulator.setValue(accumulatorMap.GetValue(coinSpend.getDenomination()));

    uint256 csHash = coinSpend.GetHash();

    bool fCached = (mapCacheValidCoinSpends.count(csHash) != 0);

    if ((!fCached && !coinSpend.Verify(accumulator)) || (fCached && mapCacheValidCoinSpends[csHash] == false)) {
        mapCacheValidCoinSpends[csHash] = false;
        return state.DoS(100, error("CheckZerocoinSpend() : CoinSpend does not verify"));
    }

    mapCacheValidCoinSpends[csHash] = true;

    if (mapCacheValidCoinSpends.size() > COINSPEND_CACHE_SIZE)
        mapCacheValidCoinSpends.clear();

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

    PublicMintChainData zeroMint;

    if (!pblocktree->ReadCoinMint(pubCoin.getValue(), zeroMint)) {
        strError = strprintf("Could not read mint with value %s from the db", pubCoin.getValue().GetHex());
        return false;
    }

    uint256 blockHash = zeroMint.GetBlockHash();

    if (!mapBlockIndex.count(blockHash)) {
        strError = strprintf("Could not find block hash %s", blockHash.ToString());
        return false;
    }

    CBlockIndex* pindex = mapBlockIndex[blockHash];

    if (!chainActive.Contains(pindex)) {
        strError = strprintf("Block %s is not part of the active chain", blockHash.ToString());
        return false;
    }

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

    int nCount = 0;
    uint64_t nTimeStart = GetTimeMicros();

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

                    nCount++;

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

    uint64_t nTimeEnd = GetTimeMicros();

    LogPrintf("Accumulated %d times and took %.2fms\n", nCount, (nTimeEnd-nTimeStart)*0.001);

    if (!pindex) {
        strError = strprintf("Last block index is null");
        return false;
    }

    accumulatorChecksum = pindex->nAccumulatorChecksum;

    if (!accumulatorMap.Load(accumulatorChecksum)) {
        strError = strprintf("Could not load Accumulators data from checksum %s of last block index %d (%s)",
                             accumulatorChecksum.GetHex(), pindex->nHeight, pindex->GetBlockHash().ToString().substr(0,8));
        return false;
    }

    accumulator.setValue(accumulatorMap.GetValue(pubCoin.getDenomination()));

    if (!accumulatorWitness.VerifyWitness(accumulator, pubCoin)) {
        strError = "Witness did not verify";
        return false;
    }

    return true;
}
