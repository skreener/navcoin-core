// Copyright (c) 2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ZEROWALLET_H
#define ZEROWALLET_H

#include "base58.h"
#include "wallet/wallet.h"
#include "random.h"

// NEEDS UNIT TEST
bool DestinationToVecRecipients(CAmount nValue, const std::string &address, vector<CRecipient> &vecSend,  bool fSubtractFeeFromAmount, bool fDonate);
bool DestinationToVecRecipients(CAmount nValue, const CTxDestination &address, vector<CRecipient> &vecSend, bool fSubtractFeeFromAmount, bool fDonate);

#endif // ZEROWALLET_H
