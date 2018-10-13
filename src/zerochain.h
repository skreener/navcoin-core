// Copyright (c) 2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ZEROCHAIN_H
#define ZEROCHAIN_H

#include "consensus/validation.h"
#include "libzerocoin/Coin.h"
#include "main.h"
#include "primitives/transaction.h"

bool TxOutToPublicCoin(const libzerocoin::ZerocoinParams *params, const CTxOut& txout, libzerocoin::PublicCoin& pubCoin, CValidationState& state);
bool CheckZerocoinMint(const libzerocoin::ZerocoinParams *params, const CTxOut& txout, CValidationState& state, libzerocoin::PublicCoin* pPubCoin = NULL);

#endif // ZEROCHAIN_H
