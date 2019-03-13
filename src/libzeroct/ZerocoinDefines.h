/**
* @file       Zerocoin.h
*
* @brief      Exceptions and constants for Zerocoin
*
* @author     Ian Miers, Christina Garman and Matthew Green
* @date       June 2013
*
* @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2017 The PIVX developers
// Copyright (c) 2018-2019 The NavCoin Core developers

#ifndef ZEROCOIN_DEFINES_H_
#define ZEROCOIN_DEFINES_H_

#include <stdexcept>

#define ZEROCOIN_DEFAULT_SECURITYLEVEL      80
#define ZEROCOIN_MIN_SECURITY_LEVEL         80
#define ZEROCOIN_MAX_SECURITY_LEVEL         80
#define ACCPROOF_KPRIME                     160
#define ACCPROOF_KDPRIME                    128
#define MAX_COINMINT_ATTEMPTS               10000
#define ZEROCOIN_MINT_PRIME_PARAM           20
#define ZEROCOIN_VERSION_STRING             "0.12"
#define ZEROCOIN_VERSION_INT                11
#define ZEROCOIN_PROTOCOL_VERSION           "1"
#define HASH_OUTPUT_BITS                    256
#define ZEROCOIN_COMMITMENT_EQUALITY_PROOF  "COMMITMENT_EQUALITY_PROOF"
#define ZEROCOIN_ACCUMULATOR_PROOF          "ACCUMULATOR_PROOF"
#define ZEROCOIN_SERIALNUMBER_PROOF         "SERIALNUMBER_PROOF"

/** Parameters used in the new protocol.
 *  !TODO: move where appropriate
 */
typedef std::vector<CBigNum> CBN_vector;
typedef std::vector<CBN_vector> CBN_matrix;

#endif /* ZEROCOIN_H_ */
