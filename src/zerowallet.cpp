// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zerowallet.h"
#include "zerotx.h"

bool DestinationToVecRecipients(CAmount nValue, const std::string &strAddress, vector<CRecipient> &vecSend, bool fSubtractFeeFromAmount, bool fDonate, bool& fRetNeedsZeroMinting, bool fPrivate, bool fReduceOutputs)
{
    CNavCoinAddress a(strAddress);

    if(!a.IsValid())
        return false;

    CTxDestination address = a.Get();

    return DestinationToVecRecipients(nValue, address, vecSend, fSubtractFeeFromAmount, fDonate, fRetNeedsZeroMinting, fPrivate, fReduceOutputs);
}

bool DestinationToVecRecipients(CAmount nValue, const CTxDestination &address, vector<CRecipient> &vecSend, bool fSubtractFeeFromAmount, bool fDonate, bool& fRetNeedsZeroMinting, bool fPrivate, bool fReduceOutputs)
{
    vecSend.clear();
    CScript scriptPubKey = GetScriptForDestination(address);

    fRetNeedsZeroMinting = false;
    map <libzerocoin::CoinDenomination, unsigned int> mapDenominations;
    CAmount minDenomination = libzerocoin::GetSmallerDenomination() * COIN;
    CAmount sumDenominations = 0;

    if(fDonate)
      CFund::SetScriptForCommunityFundContribution(scriptPubKey);

    if (scriptPubKey.IsZerocoinMint() || fPrivate) {
        for (unsigned int i = 0; i < libzerocoin::zerocoinDenomList.size(); i++) {
            sumDenominations += libzerocoin::ZerocoinDenominationToInt(libzerocoin::zerocoinDenomList[i]) * COIN;
            mapDenominations[libzerocoin::zerocoinDenomList[i]] = 0; // Initalize map
        }
    }
    if (scriptPubKey.IsZerocoinMint()) {
        CAmount nRemainingValue = nValue;
        int nCount = 0;
        unsigned int nIndex = libzerocoin::zerocoinDenomList.size() - 1;
        while (nRemainingValue >= minDenomination)
        {
            if(!fReduceOutputs && nRemainingValue >= sumDenominations)
            {
                for (unsigned int i = 0; i < libzerocoin::zerocoinDenomList.size(); i++) {
                    mapDenominations[libzerocoin::zerocoinDenomList[i]]++;
                    nCount++;
                }
                nRemainingValue -= sumDenominations;
                continue;
            }
            CAmount value = libzerocoin::ZerocoinDenominationToInt(libzerocoin::zerocoinDenomList[nIndex]) * COIN;
            if(nRemainingValue >= value) {
                mapDenominations[libzerocoin::zerocoinDenomList[nIndex]]++;
                nCount++;
                nRemainingValue -= value;
                continue;
            }
            if (nIndex == 0)
                break;
            nIndex -= 1;
        }

        if (nCount > 0)
            fRetNeedsZeroMinting = true;

        map <libzerocoin::CoinDenomination, unsigned int>::iterator it;
        for ( it = mapDenominations.begin(); it != mapDenominations.end(); it++ )
        {
            for (unsigned int i = 0; i < it->second; i++) {
                CRecipient recipient = {scriptPubKey, libzerocoin::ZerocoinDenominationToInt(it->first)  * COIN, false, ""};
                vecSend.push_back(recipient);
            }
        }
    } else {
        if (fPrivate)
            nValue = nValue - (nValue % minDenomination);
        CRecipient recipient = {scriptPubKey, nValue, fSubtractFeeFromAmount, ""};
        vecSend.push_back(recipient);
    }

    random_shuffle(vecSend.begin(), vecSend.end(), GetRandInt);

    return true;
}

bool MintVecRecipients(const std::string &strAddress, vector<CRecipient> &vecSend, bool fShowDialog)
{
    CNavCoinAddress a(strAddress);

    if(!a.IsValid())
        return false;

    CTxDestination address = a.Get();

    return MintVecRecipients(address, vecSend, fShowDialog);
}

bool MintVecRecipients(const CTxDestination &address, vector<CRecipient> &vecSend, bool fShowDialog)
{
    unsigned int i = 0;

    if (fShowDialog)
        uiInterface.ShowProgress(_("Constructing transaction..."), 0);

    for(auto& it: vecSend)
    {
        boost::this_thread::interruption_point();

        unsigned int nProgress = (i++)*100/vecSend.size();

        if (nProgress > 0 && fShowDialog)
            uiInterface.ShowProgress(_("Constructing transaction..."), nProgress);

        CScript vMintScript = GetScriptForDestination(address);
        it.scriptPubKey = vMintScript;
    }

    if (fShowDialog)
        uiInterface.ShowProgress(_("Constructing transaction..."), 100);

    return true;
}

bool PrepareAndSignCoinSpend(const BaseSignatureCreator& creator, const CScript& scriptPubKey, const CAmount& amount, CScript& sigdata)
{
    if (!scriptPubKey.IsZerocoinMint())
        return error(strprintf("Transaction output script is not a zerocoin mint."));

    string strError = "";
    libzerocoin::CoinDenomination cd = libzerocoin::AmountToZerocoinDenomination(amount);
    libzerocoin::Accumulator a(&Params().GetConsensus().Zerocoin_Params, cd);
    libzerocoin::AccumulatorWitness aw(&Params().GetConsensus().Zerocoin_Params, a,
                                       libzerocoin::PublicCoin(&Params().GetConsensus().Zerocoin_Params));
    uint256 ac;
    CTxOut txout(amount, scriptPubKey);

    libzerocoin::PublicCoin pubCoin(&Params().GetConsensus().Zerocoin_Params);

    if (!TxOutToPublicCoin(&Params().GetConsensus().Zerocoin_Params, txout, pubCoin, NULL))
        return error(strprintf("Could not convert transaction otuput to public coin"));

    if (!CalculateWitnessForMint(txout, pubCoin, a, aw, ac, strError))
        return error(strprintf("Error calculating witness for mint: %s", strError));

    if (!creator.CreateCoinSpend(&Params().GetConsensus().Zerocoin_Params, pubCoin, a, ac, aw, scriptPubKey, sigdata, strError))
        return error(strprintf("Error creating coin spend: %s", strError));

    return true;
}

bool ProduceCoinSpend(const BaseSignatureCreator& creator, const CScript& fromPubKey, SignatureData& sigdata, bool fCoinStake, CAmount amount)
{
    CScript script = fromPubKey;
    bool solved = true;
    CScript result;
    solved = PrepareAndSignCoinSpend(creator, script, amount, result);
    sigdata.scriptWitness.stack.clear();
    sigdata.scriptSig = result;

    // Test solution
    return solved;
}
