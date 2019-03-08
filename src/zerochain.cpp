// Copyright (c) 2018-2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zerochain.h"
#include "zerotx.h"

using namespace libzerocoin;

CCriticalSection cs_dummy;

bool BlockToZerocoinMints(const ZerocoinParams *params, const CBlock* block, std::vector<PublicCoin> &vPubCoins)
{
    for (auto& tx : block->vtx) {
        for (auto& out : tx.vout) {
            if (!out.IsZerocoinMint())
                continue;

            std::vector<unsigned char> c; CPubKey p; std::vector<unsigned char> i;  std::vector<unsigned char> a;  std::vector<unsigned char> aco;
            if(!out.scriptPubKey.ExtractZerocoinMintData(p, c, i, a, aco))
                return error("BlockToZerocoinMints(): Could not extract Zerocoin mint data");

            CBigNum pid;
            pid.setvch(i);

            CBigNum oa;
            oa.setvch(a);

            CBigNum ac;
            ac.setvch(aco);

            PublicCoin checkPubCoin(params, CBigNum(c), p, pid, oa, ac);
            vPubCoins.push_back(checkPubCoin);
        }
    }

    return true;
}

bool CheckZerocoinMint(const ZerocoinParams *params, const CTxOut& txout, const CCoinsViewCache& view, CValidationState& state, std::vector<std::pair<CBigNum, PublicMintChainData>> vSeen, PublicCoin* pPubCoin, bool fCheck, bool fFast)
{
    PublicCoin pubCoin(params);
    if(!TxOutToPublicCoin(params, txout, pubCoin, &state, false))
        return state.DoS(100, error("CheckZerocoinMint(): TxOutToPublicCoin() failed"));

    if (pPubCoin)
        *pPubCoin = pubCoin;

    if (fCheck && !pubCoin.isValid(fFast))
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

bool CheckZerocoinSpend(const ZerocoinParams *params, const CTxIn& txin, const CCoinsViewCache& view, CValidationState& state, std::vector<std::pair<CBigNum, uint256>> vSeen, CoinSpend* pCoinSpend, Accumulator* pAccumulator, bool fSpendCheck)
{
    CoinSpend coinSpend(params);

    if(!TxInToCoinSpend(params, txin, coinSpend))
        return state.DoS(100, error("CheckZerocoinSpend() : TxInToCoinSpend() failed"));

    if (pCoinSpend)
        *pCoinSpend = coinSpend;

    uint256 blockAccumulatorHash = coinSpend.getBlockAccumulatorHash();

    if (!mapBlockIndex.count(blockAccumulatorHash))
        return state.DoS(100, error("CheckZerocoinSpend() : coinspend refers an invalid block hash %s", blockAccumulatorHash.ToString()));

    CBlockIndex* pindex = mapBlockIndex[blockAccumulatorHash];

    if (!chainActive.Contains(pindex))
        return state.DoS(20, error("CheckZerocoinSpend() : coinspend refers a block not contained in the main chain"));

    Accumulator accumulator(params);
    accumulator.setValue(pindex->nAccumulatorValue);

    if (pAccumulator)
        *pAccumulator = accumulator;

    if (fSpendCheck) {
        if (!VerifyCoinSpendCache(coinSpend, accumulator))
            return state.DoS(100, error("CheckZerocoinSpend() : CoinSpend does not verify"));
    }

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

bool VerifyCoinSpendCache(const CoinSpend& coinSpend, const Accumulator &accumulator)
{
    LOCK(cs_coinspend_cache);

    return VerifyCoinSpend(coinSpend, accumulator, true);
}

bool VerifyCoinSpendNoCache(const CoinSpend& coinSpend, const Accumulator &accumulator)
{
    return VerifyCoinSpend(coinSpend, accumulator, false);
}

bool VerifyCoinSpend(const CoinSpend& coinSpend, const Accumulator &accumulator, bool fWriteToCache)
{
    uint256 csHash = coinSpend.GetHash();
    bool fCached = (mapCacheValidCoinSpends.count(csHash) != 0);

    if ((!fCached && !coinSpend.Verify(accumulator)) || (fCached && mapCacheValidCoinSpends[csHash] == false)) {
        if (fWriteToCache)
            mapCacheValidCoinSpends[csHash] = false;
        return false;
    }

    if (fWriteToCache)
        mapCacheValidCoinSpends[csHash] = true;

    if (fWriteToCache && mapCacheValidCoinSpends.size() > COINSPEND_CACHE_SIZE)
        mapCacheValidCoinSpends.clear();

    return true;
}

bool CountMintsFromHeight(unsigned int nInitialHeight, unsigned int& nRet)
{
    nRet = 0;
    CBlockIndex* pindex = chainActive[nInitialHeight];

    while(pindex)
    {
        CBlock block;
        if (!ReadBlockFromDisk(block, pindex, Params().GetConsensus()))
            return false;

        for (auto& tx: block.vtx)
            for (CTxOut& out: tx.vout)
                if(out.IsZerocoinMint())
                    nRet++;

        pindex = chainActive.Next(pindex);
    }

    return true;
}

bool CalculateWitnessForMint(const CTxOut& txout, const libzerocoin::PublicCoin& pubCoin, Accumulator& accumulator, AccumulatorWitness& accumulatorWitness, CBigNum& accumulatorValue, uint256& blockAccumulatorHash, std::string& strError, int nRequiredMints, int nMaxHeight)
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

    if(pindex->nAccumulatorValue != 0)
        accumulator.setValue(pindex->nAccumulatorValue);

    accumulatorWitness.resetValue(accumulator, pubCoin);

    int nCount = 0;

    if (chainActive.Next(pindex)) {
        pindex = chainActive.Next(pindex);
        while (pindex && pindex->nHeight <= nMaxHeight) {
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

                    nCount++;

                    accumulatorWitness.AddElement(pubCoinOut);
                    accumulator.accumulate(pubCoinOut);
                }
            }

            accumulatorValue = pindex->nAccumulatorValue;
            blockAccumulatorHash = pindex->GetBlockHash();

            assert(accumulator.getValue() == accumulatorValue);

            if(!chainActive.Next(pindex) || (nRequiredMints > 0 && nCount >= nRequiredMints))
                break;

            pindex = chainActive.Next(pindex);
        }
    }

    accumulator.setValue(accumulatorValue);

    if (!accumulatorWitness.VerifyWitness(accumulator, pubCoin)) {
        strError = "Witness did not verify";
        return false;
    }

    return true;
}


bool VerifyZeroCTBalance(const ZerocoinParams *params, const CTransaction& tx, const CCoinsViewCache& view)
{
    if (!tx.IsZeroCT())
        return false;

    CDataStream ss(tx.vchTxSig, SER_NETWORK, PROTOCOL_VERSION);
    libzerocoin::SerialNumberProofOfKnowledge snpok(&params->coinCommitmentGroup);

    ss >> snpok;

    CBigNum bnInputs = 1;
    CBigNum bnOutputs = 1;

    for (auto &in: tx.vin)
    {
        if (in.scriptSig.IsZerocoinSpend())
        {
            libzerocoin::CoinSpend spend(params);

            if (!ScriptToCoinSpend(params, in.scriptSig, spend))
                return false;

            bnInputs = bnInputs.mul_mod(spend.getAmountCommitment(), params->coinCommitmentGroup.modulus);
        }
        else
        {
            CAmount inputValue = view.GetOutputFor(in).nValue;
            bnInputs = bnInputs.mul_mod(params->coinCommitmentGroup.g2.pow_mod(inputValue, params->coinCommitmentGroup.modulus), params->coinCommitmentGroup.modulus);
        }
    }

    for (auto &out: tx.vout)
    {
        if (out.IsZerocoinMint())
        {
            libzerocoin::PublicCoin pc(params);

            if (!TxOutToPublicCoin(params, out, pc))
                return false;

            bnOutputs = bnOutputs.mul_mod(pc.getAmountCommitment(), params->coinCommitmentGroup.modulus);
        }
        else
        {
            bnOutputs = bnOutputs.mul_mod(params->coinCommitmentGroup.g2.pow_mod(out.nValue, params->coinCommitmentGroup.modulus), params->coinCommitmentGroup.modulus);
        }
    }

    CBigNum bnPubKey = bnInputs.mul_mod(bnOutputs.inverse(params->coinCommitmentGroup.modulus), params->coinCommitmentGroup.modulus);

    return snpok.Verify(bnPubKey, tx.GetHashAmountSig());
}
