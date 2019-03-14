/**
* @file       tutorial.cpp
*
* @brief      Simple tutorial program to illustrate Zerocoin usage.
*
* @author     Ian Miers, Christina Garman and Matthew Green
* @date       June 2013
*
* @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2017-2018 The PIVX developers
// Copyright (c) 2018 The NavCoin Core developers

#include <boost/test/unit_test.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <exception>
#include "streams.h"
#include "libzeroct/bignum.h"
#include "libzeroct/BulletproofsRangeproof.h"
#include "libzeroct/ParamGeneration.h"
#include "libzeroct/Coin.h"
#include "libzeroct/CoinSpend.h"
#include "libzeroct/Accumulator.h"
#include "test/test_navcoin.h"

using namespace std;

#define COINS_TO_ACCUMULATE     5
#define DUMMY_TRANSACTION_HASH  0 // in real life these would be uint256 hashes
#define DUMMY_ACCUMULATOR_ID    0 // in real life these would be uint256 hashes

//
// We generated this for testing only. Don't use it in production!
//
#define TUTORIAL_TEST_MODULUS   "a8852ebf7c49f01cd196e35394f3b74dd86283a07f57e0a262928e7493d4a3961d93d93c90ea3369719641d626d28b9cddc6d9307b9aabdbffc40b6d6da2e329d079b4187ff784b2893d9f53e9ab913a04ff02668114695b07d8ce877c4c8cac1b12b9beff3c51294ebe349eca41c24cd32a6d09dd1579d3947e5c4dcc30b2090b0454edb98c6336e7571db09e0fdafbd68d8f0470223836e90666a5b143b73b9cd71547c917bf24c0efc86af2eba046ed781d9acb05c80f007ef5a0a5dfca23236f37e698e8728def12554bc80f294f71c040a88eff144d130b24211016a97ce0f5fe520f477e555c9997683d762aff8bd1402ae6938dd5c994780b1bf6aa7239e9d8101630ecfeaa730d2bbc97d39beb057f016db2e28bf12fab4989c0170c2593383fd04660b5229adcd8486ba78f6cc1b558bcd92f344100dff239a8c00dbc4c2825277f24bdd04475bcc9a8c39fd895eff97c1967e434effcb9bd394e0577f4cf98c30d9e6b54cd47d6e447dcf34d67e48e4421691dbe4a7d9bd503abb9"

//
// The following routine exercises most of the core functions of
// the library. We've commented it as well as possible to give
// you the flavor of how libzeroct works.
//
// For details on Zerocoin integration, see the libzeroct wiki
// at: https://github.com/Zerocoin/libzeroct/wiki
//

bool
ZerocoinTutorial()
{
    // The following simple code illustrates the call flow for Zerocoin
    // applications. In a real currency network these operations would
    // be split between individual payers/payees, network nodes and miners.
    //
    // For each call we specify the participant who would use it.

    // Zerocoin uses exceptions (based on the runtime_error class)
    // to indicate all sorts of problems. Always remember to catch them!

    try {

        /********************************************************************/
        // What is it:      Parameter loading
        // Who does it:     ALL ZEROCOIN PARTICIPANTS
        // What it does:    Loads a trusted Zerocoin modulus "N" and
        //                  generates all associated parameters from it.
        //                  We use a hardcoded "N" that we generated using
        //                  the included 'paramgen' utility.
        /********************************************************************/
        cout << "Successfully loaded parameters." << endl;

        // Load a test modulus from our hardcoded string (above)
        CBigNum testModulus;
        testModulus.SetHex(std::string(TUTORIAL_TEST_MODULUS));

        // Set up the Zerocoin Params object
        libzeroct::ZeroCTParams* params = new libzeroct::ZeroCTParams(testModulus);

        CKey privKey;
        CPubKey pubKey;

        // Generate a new Key Pair
        privKey.MakeNewKey(false);
        pubKey = privKey.GetPubKey();

        // Generate obfuscation parameters and a blinding commitment
        CBigNum obfuscation_j1 = CBigNum::randBignum(params->coinCommitmentGroup.groupOrder);
        CBigNum obfuscation_j2 = CBigNum::randBignum(params->coinCommitmentGroup.groupOrder);
        CBigNum obfuscation_k1 = CBigNum::randBignum(params->coinCommitmentGroup.groupOrder);
        CBigNum obfuscation_k2 = CBigNum::randBignum(params->coinCommitmentGroup.groupOrder);
        CBigNum blindingCommitment1 = params->coinCommitmentGroup.g.pow_mod(obfuscation_j1, params->coinCommitmentGroup.modulus).mul_mod(params->coinCommitmentGroup.h.pow_mod(obfuscation_k1, params->coinCommitmentGroup.modulus), params->coinCommitmentGroup.modulus);
        CBigNum blindingCommitment2 = params->coinCommitmentGroup.g.pow_mod(obfuscation_j2, params->coinCommitmentGroup.modulus).mul_mod(params->coinCommitmentGroup.h.pow_mod(obfuscation_k2, params->coinCommitmentGroup.modulus), params->coinCommitmentGroup.modulus);

        libzeroct::ObfuscationValue obfuscation_j = make_pair(obfuscation_j1, obfuscation_j2);
        libzeroct::ObfuscationValue obfuscation_k = make_pair(obfuscation_k1, obfuscation_k2);
        libzeroct::BlindingCommitment blindingCommitment = make_pair(blindingCommitment1, blindingCommitment2);

        /********************************************************************/
        // What is it:      Coin generation
        // Who does it:     ZEROCOIN CLIENTS
        // What it does:    Generates a new 'zerocoin' coin using the
        //                  public parameters. Once generated, the client
        //                  will transmit the public portion of this coin
        //                  in a ZEROCOIN_MINT transaction. The inputs
        //                  to this transaction must add up to the zerocoin
        //                  denomination plus any transaction fees.
        /********************************************************************/

        // The following constructor does all the work of minting a brand
        // new zerocoin. You should embed this into a Zerocoin 'MINT' transaction
        // along with a series of currency inputs totaling the assigned value of one zerocoin.
        CBigNum rpdata;

        libzeroct::PublicCoin pubCoin(params, pubKey, blindingCommitment, "test_payment_id", COIN, &rpdata);

        libzeroct::PrivateCoin newCoin(params, privKey, pubCoin.getPubKey(), blindingCommitment, pubCoin.getValue(), pubCoin.getPaymentId(), pubCoin.getAmount());

        if (!newCoin.isValid()) {
            cout << "Error calculating private parameters of new zerocoin." << endl;
            return false;
        }

        if(newCoin.getPublicCoin().getValue() != pubCoin.getValue()) {
            cout << "The private coin and the public coin do not share the same commitment value." << endl;
            return false;
        }

        if(newCoin.getPaymentId() != "test_payment_id") {
            cout << "The private coin and the public coin do not share the same payment id. " << newCoin.getPaymentId() << endl;
            return false;
        }

        if(newCoin.getAmount() != COIN) {
            cout << "The private coin and the public coin do not share the same amount. " << newCoin.getAmount() << endl;
            return false;
        }

        BulletproofsRangeproof bprp(&params->coinCommitmentGroup);

        CBN_vector values;
        CBN_vector gammas;

        values.push_back(newCoin.getAmount());
        gammas.push_back(rpdata);

        bprp.Prove(values, gammas);

        CBN_matrix mValueCommitments;
        std::vector<BulletproofsRangeproof> proofs;
        CBN_vector valueCommitments = bprp.GetValueCommitments();

        proofs.push_back(bprp);
        mValueCommitments.push_back(valueCommitments);

        if (!VerifyBulletproof(&params->coinCommitmentGroup, proofs, mValueCommitments)) {
            cout << "The range proof could not get verified" << endl;
            return false;
        }

        CDataStream serializedRangeProof(SER_NETWORK, PROTOCOL_VERSION);
        serializedRangeProof << bprp;

        BulletproofsRangeproof newbprp(&params->coinCommitmentGroup, serializedRangeProof);

        std::vector<BulletproofsRangeproof> proofs2;
        proofs2.push_back(newbprp);

        if (!VerifyBulletproof(&params->coinCommitmentGroup, proofs2, mValueCommitments)) {
            cout << "The serialized range proof could not get verified" << endl;
            //return false;
        }

        // Serialize the public coin to a CDataStream object.
        CDataStream serializedCoin(SER_NETWORK, PROTOCOL_VERSION);
        serializedCoin << pubCoin;

        /********************************************************************/
        // What is it:      Coin verification
        // Who does it:     TRANSACTION VERIFIERS
        // What it does:    Verifies the structure of a zerocoin obtained from
        //                  a ZEROCOIN_MINT transaction. All coins must be
        //                  verified before you operate on them.
        //                  Note that this is only part of the transaction
        //                  verification process! The client must also check
        //                  that (1) the inputs to the transaction are valid
        //                  and add up to the value of one zerocoin, (2) that
        //                  this particular zerocoin has not been minted before.
        /********************************************************************/

        // Deserialize the public coin into a fresh object.
        libzeroct::PublicCoin pubCoinNew(params, serializedCoin);

        // Now make sure the coin is valid.
        if (!pubCoinNew.isValid()) {
            // If this returns false, don't accept the coin for any purpose!
            // Any ZEROCOIN_MINT with an invalid coin should NOT be
            // accepted as a valid transaction in the block chain.
            cout << "Error: coin is not valid!";
        }

        cout << "Deserialized and verified the coin." << endl;


        /********************************************************************/
        // What is it:      Accumulator computation
        // Who does it:     ZEROCOIN CLIENTS & TRANSACTION VERIFIERS
        // What it does:    Collects a number of PublicCoin values drawn from
        //                  the block chain and calculates an accumulator.
        //                  This accumulator is incrementally computable;
        //                  you can stop and serialize it at any point
        //                  then continue accumulating new transactions.
        //                  The accumulator is also order-independent, so
        //                  the same coins can be accumulated in any order
        //                  to give the same result.
        //                  WARNING: do not accumulate the same coin twice!
        /********************************************************************/

        // Create an empty accumulator object
        libzeroct::Accumulator accumulator(params);

        // Add several coins to it (we'll generate them here on the fly).
        for (uint32_t i = 0; i < COINS_TO_ACCUMULATE; i++) {
            libzeroct::PublicCoin testCoin(params, pubKey, blindingCommitment, "", COIN, &rpdata);
            accumulator += testCoin;
        }

        // Serialize the accumulator object.
        //
        // If you're using Accumulator Checkpoints, each miner would
        // start by deserializing the accumulator checkpoint from the
        // previous block (or creating a new Accumulator if no previous
        // block exists). It will then add all the coins in the new block,
        // then serialize the resulting Accumulator object to obtain the
        // new checkpoint. All block verifiers should do the same thing
        // to check their work.
        CDataStream serializedAccumulator(SER_NETWORK, PROTOCOL_VERSION);
        serializedAccumulator << accumulator;

        // Deserialize the accumulator object
        libzeroct::Accumulator newAccumulator(params, serializedAccumulator);

        // We can now continue accumulating things into the accumulator
        // we just deserialized. For example, let's put in the coin
        // we generated up above.
        newAccumulator += pubCoinNew;

        cout << "Successfully accumulated coins." << endl;

        /********************************************************************/
        // What is it:      Coin spend
        // Who does it:     ZEROCOIN CLIENTS
        // What it does:    Create a new transaction that spends a Zerocoin.
        //                  The user first authors a transaction specifying
        //                  a set of destination addresses (outputs). They
        //                  next compute an Accumulator over all coins in the
        //                  block chain (see above) and a Witness based on the
        //                  coin to be spent. Finally they instantiate a CoinSpend
        //                  object that 'signs' the transaction with a special
        //                  zero knowledge signature of knowledge over the coin
        //                  data, Witness and Accumulator.
        /********************************************************************/

        // We are going to spend the coin "newCoin" that we generated at the
        // top of this function.
        //
        // We'll use the accumulator we constructed above. This contains
        // a set of coins, but does NOT include the coin "newCoin".
        //
        // To generate the witness, we start with this accumulator and
        // add the public half of the coin we want to spend.
        libzeroct::AccumulatorWitness witness(params, accumulator, newCoin.getPublicCoin());

        // Add the public half of "newCoin" to the Accumulator itself.
        accumulator += newCoin.getPublicCoin();

        // At this point we should generate a ZEROCOIN_SPEND transaction to
        // send to the network. This network should include a set of outputs
        // totalling to the value of one zerocoin (minus transaction fees).

        CBigNum r;

        // Construct the CoinSpend object. This acts like a signature on the
        // transaction.
        libzeroct::CoinSpend spend(params, newCoin, accumulator, 0, witness, 0, libzeroct::SpendType::SPEND, obfuscation_j, obfuscation_k, r);

        // This is a sanity check. The CoinSpend object should always verify,
        // but why not check before we put it onto the wire?
        if (!spend.Verify(accumulator)) {
            cout << "ERROR: Our new CoinSpend transaction did not verify!" << endl;
            return false;
        }

        libzeroct::CoinSpend stake(params, newCoin, accumulator, 0, witness, 0, libzeroct::SpendType::STAKE, obfuscation_j, obfuscation_k, r);
        if (!stake.Verify(accumulator)) {
            cout << "ERROR: Our new CoinStake transaction did not verify!" << endl;
            return false;
        }

        // Serialize the CoinSpend object into a buffer.
        CDataStream serializedCoinSpend(SER_NETWORK, PROTOCOL_VERSION);
        serializedCoinSpend << spend;

        CDataStream serializedCoinStake(SER_NETWORK, PROTOCOL_VERSION);
        serializedCoinStake << stake;

        cout << "Successfully generated a CoinSpend and a CoinStake transaction." << endl;

        /********************************************************************/
        // What is it:      Coin spend verification
        // Who does it:     ALL PARTIES
        // What it does:    Verifies that a CoinSpend signature is correct
        //                  with respect to a ZEROCOIN_SPEND transaction hash.
        //                  The client must also extract the serial number from
        //                  the CoinSpend and verify that this serial number has
        //                  not previously appeared in another ZEROCOIN_SPEND
        //                  transaction.
        /********************************************************************/

        // Deserialize the CoinSpend intro a fresh object
        libzeroct::CoinSpend newSpend(params, serializedCoinSpend);
        libzeroct::CoinSpend newStake(params, serializedCoinStake);

        // Create a new metadata object to contain the hash of the received
        // ZEROCOIN_SPEND transaction. If we were a real client we'd actually
        // compute the hash of the received transaction here.

        // If we were a real client we would now re-compute the Accumulator
        // from the information given in the ZEROCOIN_SPEND transaction.
        // For our purposes we'll just use the one we calculated above.
        //
        // Verify that the spend is valid with respect to the Accumulator
        // and the Metadata
        if (!newSpend.Verify(accumulator)) {
            cout << "ERROR: The serialized CoinSpend transaction did not verify!" << endl;
            return false;
        }

        if (!newStake.Verify(accumulator)) {
            cout << "ERROR: The serialized CoinStake transaction did not verify!" << endl;
            return false;
        }

        // Pull the serial number out of the CoinSpend object. If we
        // were a real Zerocoin client we would now check that the serial number
        // has not been spent before (in another ZEROCOIN_SPEND) transaction.
        // The serial number is stored as a CBigNum.
        CBigNum serialNumber = newSpend.getCoinSerialNumber();

        cout << "Successfully verified the CoinSpend and CoinStake proofs." << endl;
        cout << endl << "Coin serial number is:" << endl << serialNumber << endl;

        // We're done
        return true;

    } catch (runtime_error &e) {
        cout << e.what() << endl;
        return false;
    }

    return false;
}

BOOST_FIXTURE_TEST_SUITE(tutorial_libzeroct_tests, BasicTestingSetup)
BOOST_AUTO_TEST_CASE(tutorial_libzeroct_tests)
{
    cout << "libzeroct v" << ZEROCOIN_VERSION_STRING << " tutorial." << endl << endl;

    ZerocoinTutorial();
}
BOOST_AUTO_TEST_SUITE_END()
