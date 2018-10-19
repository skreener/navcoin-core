// Copyright (c) 2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ZEROCHAIN_H
#define ZEROCHAIN_H

#include "chainparams.h"
#include "consensus/validation.h"
#include "libzerocoin/Coin.h"
#include "main.h"
#include "primitives/transaction.h"

bool TxOutToPublicCoin(const libzerocoin::ZerocoinParams *params, const CTxOut& txout, libzerocoin::PublicCoin& pubCoin, CValidationState& state);
bool CheckZerocoinMint(const libzerocoin::ZerocoinParams *params, const CTxOut& txout, CValidationState& state, std::vector<std::pair<CBigNum, uint256>> vSeen = std::vector<std::pair<CBigNum, uint256>>(), libzerocoin::PublicCoin* pPubCoin = NULL);
bool BlockToZeroCoinMints(const libzerocoin::ZerocoinParams *params, const CBlock* block, std::vector<libzerocoin::PublicCoin> &vPubCoins);
bool CountMintsFromHeight(unsigned int nInitialHeight, libzerocoin::CoinDenomination denom, unsigned int& nRet);

#endif // ZEROCHAIN_H
