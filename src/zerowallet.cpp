// Copyright (c) 2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zerowallet.h"

bool DestinationToVecRecipients(CAmount nValue, const std::string &strAddress, vector<CRecipient> &vecSend, bool fSubtractFeeFromAmount, bool fDonate)
{
    CNavCoinAddress a(strAddress);

    if(!a.IsValid())
        return false;

    CTxDestination address = a.Get();

    return DestinationToVecRecipients(nValue, address, vecSend, fSubtractFeeFromAmount, fDonate);
}

bool DestinationToVecRecipients(CAmount nValue, const CTxDestination &address, vector<CRecipient> &vecSend, bool fSubtractFeeFromAmount, bool fDonate)
{
    vecSend.clear();
    CScript scriptPubKey = GetScriptForDestination(address);

    if(fDonate)
      CFund::SetScriptForCommunityFundContribution(scriptPubKey);

    if (scriptPubKey.IsZerocoinMint()) {
        CAmount sumDenominations = 0;
        map <libzerocoin::CoinDenomination, unsigned int> mapDenominations;
        CAmount minDenomination = MAX_MONEY / COIN;
        for (unsigned int i = 0; i < libzerocoin::zerocoinDenomList.size(); i++) {
            sumDenominations += libzerocoin::ZerocoinDenominationToInt(libzerocoin::zerocoinDenomList[i]) * COIN;
            mapDenominations[libzerocoin::zerocoinDenomList[i]] = 0; // Initalize map
            minDenomination = min(minDenomination, libzerocoin::ZerocoinDenominationToInt(libzerocoin::zerocoinDenomList[i]));
        }
        minDenomination *= COIN;
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
        map <libzerocoin::CoinDenomination, unsigned int>::iterator it;
        for ( it = mapDenominations.begin(); it != mapDenominations.end(); it++ )
        {
            for (unsigned int i = 0; i < it->second; i++) {
                CRecipient recipient = {GetScriptForDestination(address), libzerocoin::ZerocoinDenominationToInt(it->first)  * COIN, fSubtractFeeFromAmount, ""};
                vecSend.push_back(recipient);
            }
        }
    } else {
        CRecipient recipient = {scriptPubKey, nValue, fSubtractFeeFromAmount, ""};
        vecSend.push_back(recipient);
    }

    random_shuffle(vecSend.begin(), vecSend.end(), GetRandInt);

    return true;
}
