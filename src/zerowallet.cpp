// Copyright (c) 2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zerowallet.h"

bool DestinationToVecRecipients(CAmount nValue, const std::string &strAddress, vector<CRecipient> &vecSend, bool fSubtractFeeFromAmount, bool fDonate, bool& fRetNeedsZeroMinting, bool fPrivate)
{
    CNavCoinAddress a(strAddress);

    if(!a.IsValid())
        return false;

    CTxDestination address = a.Get();

    return DestinationToVecRecipients(nValue, address, vecSend, fSubtractFeeFromAmount, fDonate, fRetNeedsZeroMinting, fPrivate);
}

bool DestinationToVecRecipients(CAmount nValue, const CTxDestination &address, vector<CRecipient> &vecSend, bool fSubtractFeeFromAmount, bool fDonate, bool& fRetNeedsZeroMinting, bool fPrivate)
{
    vecSend.clear();
    CScript scriptPubKey = GetScriptForDestination(address);

    fRetNeedsZeroMinting = false;
    map <libzerocoin::CoinDenomination, unsigned int> mapDenominations;
    CAmount minDenomination = MAX_MONEY / COIN;
    CAmount sumDenominations = 0;

    if(fDonate)
      CFund::SetScriptForCommunityFundContribution(scriptPubKey);

    if (scriptPubKey.IsZerocoinMint() || fPrivate) {
        for (unsigned int i = 0; i < libzerocoin::zerocoinDenomList.size(); i++) {
            sumDenominations += libzerocoin::ZerocoinDenominationToInt(libzerocoin::zerocoinDenomList[i]) * COIN;
            mapDenominations[libzerocoin::zerocoinDenomList[i]] = 0; // Initalize map
            minDenomination = min(minDenomination, libzerocoin::ZerocoinDenominationToInt(libzerocoin::zerocoinDenomList[i]));
        }
        minDenomination *= COIN;
    }
    if (scriptPubKey.IsZerocoinMint()) {
        CAmount nRemainingValue = nValue;
        int nCount = 0;
        while (nRemainingValue >= minDenomination)
        {
            if(nRemainingValue >= sumDenominations)
            {
                for (unsigned int i = 0; i < libzerocoin::zerocoinDenomList.size(); i++) {
                    mapDenominations[libzerocoin::zerocoinDenomList[i]]++;
                    nCount++;
                }
                nRemainingValue -= sumDenominations;
                continue;
            }
            for (unsigned int i = 0; i < libzerocoin::zerocoinDenomList.size(); i++) {
                unsigned int nIndex = libzerocoin::zerocoinDenomList.size() - i - 1;
                CAmount value = libzerocoin::ZerocoinDenominationToInt(libzerocoin::zerocoinDenomList[nIndex]) * COIN;
                if(nRemainingValue >= value) {
                    mapDenominations[libzerocoin::zerocoinDenomList[nIndex]]++;
                    nCount++;
                    nRemainingValue -= value;
                    continue;
                }
            }
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

bool MintVecRecipients(const std::string &strAddress, vector<CRecipient> &vecSend)
{
    CNavCoinAddress a(strAddress);

    if(!a.IsValid())
        return false;

    CTxDestination address = a.Get();

    return MintVecRecipients(address, vecSend);
}

bool MintVecRecipients(const CTxDestination &address, vector<CRecipient> &vecSend)
{
    unsigned int i = 0;

    uiInterface.ShowProgress(_("Constructing transaction..."), 0);

    for(auto& it: vecSend)
    {
        unsigned int nProgress = (i++)*100/vecSend.size();

        if (nProgress > 0)
            uiInterface.ShowProgress(_("Constructing transaction..."), nProgress);

        CScript vMintScript = GetScriptForDestination(address);
        it.scriptPubKey = vMintScript;
    }

    uiInterface.ShowProgress(_("Constructing transaction..."), 100);

    return true;
}
