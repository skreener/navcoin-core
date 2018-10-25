// Copyright (c) 2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ZEROCHAIN_H
#define ZEROCHAIN_H

#include "chainparams.h"
#include "consensus/validation.h"
#include "libzerocoin/Coin.h"
#include "libzerocoin/CoinSpend.h"
#include "main.h"
#include "primitives/transaction.h"

#define BIGNUM_SIZE   4

using namespace libzerocoin;

bool TxOutToPublicCoin(const ZerocoinParams *params, const CTxOut& txout, PublicCoin& pubCoin, CValidationState* state = NULL);
bool CheckZerocoinMint(const ZerocoinParams *params, const CTxOut& txout, CValidationState& state, std::vector<std::pair<CBigNum, uint256>> vSeen = std::vector<std::pair<CBigNum, uint256>>(), PublicCoin* pPubCoin = NULL);
bool BlockToZerocoinMints(const ZerocoinParams *params, const CBlock* block, std::vector<PublicCoin> &vPubCoins);
bool CountMintsFromHeight(unsigned int nInitialHeight, CoinDenomination denom, unsigned int& nRet);
bool CalculateWitnessForMint(const CTxOut& txout, const PublicCoin& pubCoin, Accumulator& accumulator, AccumulatorWitness& AccumulatorWitness, uint256& AccumulatorChecksum, std::string& strError);
bool TxInToCoinSpend(const ZerocoinParams *params, const CTxIn& txin, CoinSpend& coinSpend, CValidationState* state = NULL);
bool CheckZerocoinSpend(const ZerocoinParams *params, const CTxIn& txin, CValidationState& state, std::vector<std::pair<CBigNum, uint256>> vSeen = std::vector<std::pair<CBigNum, uint256>>(), CoinSpend* pCoinSpend = NULL);
#endif // ZEROCHAIN_H
