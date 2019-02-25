/**
* @file       Params.h
*
* @brief      Parameter classes for Zerocoin.
*
* @author     Ian Miers, Christina Garman and Matthew Green
* @date       June 2013
*
* @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2017 The PIVX developers

#ifndef PARAMS_H_
#define PARAMS_H_

#include "bignum.h"
#include "ZerocoinDefines.h"

namespace libzerocoin {

class IntegerGroupParams {
public:
	/** @brief Integer group class, default constructor
	*
	* Allocates an empty (uninitialized) set of parameters.
	**/
	IntegerGroupParams();

  /**
   * Generates a random group element
   * @return a random element in the group.
   */
  CBigNum randomElement() const;
  bool initialized;

  /**
   * A generator for the group.
   */
  CBigNum g;

  /**
   * A list of ZKP_N + ZKP_PAD generators for the group.
   */
  CBN_vector gis;

  /**
   * A second generator for the group.
   * Note log_g(h), log_g(g2), log_g2(g), log_g2(h), log_h(g) and log_h(g2) must
   * be unknown.
   */
  CBigNum h;

  /**
   * A list of ZKP_N + ZKP_PAD second generators for the group.
   */
  CBN_vector his;

  /**
   * A third generator for the group.
   * Note log_g(h), log_g(g2), log_g2(g), log_g2(h), log_h(g) and log_h(g2) must
   * be unknown.
   */
  CBigNum g2;

  /**
   * A random generator in Zp used for Bulletproofs inner Product
   */
  CBigNum u_inner_prod;

  /**
   * The modulus for the group.
   */
  CBigNum modulus;

  /**
   * The order of the group
   */
  CBigNum groupOrder;

	ADD_SERIALIZE_METHODS;
  template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		    READWRITE(initialized);
		    READWRITE(g);
        READWRITE(g2);
        READWRITE(h);
        READWRITE(gis);
        READWRITE(modulus);
		    READWRITE(groupOrder);
	}	
};

class AccumulatorAndProofParams {
public:
	/** @brief Construct a set of Zerocoin parameters from a modulus "N".
	* @param N                A trusted RSA modulus
	* @param securityLevel    A security level expressed in symmetric bits (default 80)
	*
	* Allocates and derives a set of Zerocoin parameters from
	* a trustworthy RSA modulus "N". This routine calculates all
	* of the remaining parameters (group descriptions etc.) from N
	* using a verifiable, deterministic procedure.
	*
	* Note: this constructor makes the fundamental assumption that "N"
	* encodes a valid RSA-style modulus of the form "e1 * e2" where
	* "e1" and "e2" are safe primes. The factors "e1", "e2" MUST NOT
	* be known to any party, or the security of Zerocoin is
	* compromised. The integer "N" must be a MINIMUM of 1024
	* in length. 3072 bits is strongly recommended.
	**/
	AccumulatorAndProofParams();

	//AccumulatorAndProofParams(CBigNum accumulatorModulus);

	bool initialized;

	/**
	 * Modulus used for the accumulator.
	 * Product of two safe primes who's factorization is unknown.
	 */
	CBigNum accumulatorModulus;

	/**
	 * The initial value for the accumulator
	 * A random Quadratic residue mod n thats not 1
	 */
	CBigNum accumulatorBase;

	/**
	 * Lower bound on the value for committed coin.
	 * Required by the accumulator proof.
	 */
	CBigNum minCoinValue;

	/**
	 * Upper bound on the value for a comitted coin.
	 * Required by the accumulator proof.
	 */
	CBigNum maxCoinValue;

	/**
	 * The second of two groups used to form a commitment to
	 * a coin (which it self is a commitment to a serial number).
	 * This one differs from serialNumberSokCommitment due to
	 * restrictions from Camenisch and Lysyanskaya's paper.
	 */
	IntegerGroupParams accumulatorPoKCommitmentGroup;

	/**
	 * Hidden order quadratic residue group mod N.
	 * Used in the accumulator proof.
	 */
	IntegerGroupParams accumulatorQRNCommitmentGroup;

	/**
	 * Security parameter.
	 * Bit length of the challenges used in the accumulator proof.
	 */
	uint32_t k_prime;

	/**
	 * Security parameter.
	 * The statistical zero-knowledgeness of the accumulator proof.
	 */
	uint32_t k_dprime;
	ADD_SERIALIZE_METHODS;
  template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
	    READWRITE(initialized);
	    READWRITE(accumulatorModulus);
	    READWRITE(accumulatorBase);
	    READWRITE(accumulatorPoKCommitmentGroup);
	    READWRITE(accumulatorQRNCommitmentGroup);
	    READWRITE(minCoinValue);
	    READWRITE(maxCoinValue);
	    READWRITE(k_prime);
	    READWRITE(k_dprime);
  }
};

class ZerocoinParams {
public:
	/** @brief Construct a set of Zerocoin parameters from a modulus "N".
	* @param N                A trusted RSA modulus
	* @param securityLevel    A security level expressed in symmetric bits (default 80)
	*
	* Allocates and derives a set of Zerocoin parameters from
	* a trustworthy RSA modulus "N". This routine calculates all
	* of the remaining parameters (group descriptions etc.) from N
	* using a verifiable, deterministic procedure.
	*
	* Note: this constructor makes the fundamental assumption that "N"
	* encodes a valid RSA-style modulus of the form "e1 * e2" where
	* "e1" and "e2" are safe primes. The factors "e1", "e2" MUST NOT
	* be known to any party, or the security of Zerocoin is
	* compromised. The integer "N" must be a MINIMUM of 1024
	* in length. 3072 bits is strongly recommended.
	**/
	ZerocoinParams(CBigNum accumulatorModulus,
	       uint32_t securityLevel = ZEROCOIN_DEFAULT_SECURITYLEVEL);

  ZerocoinParams() { this->initialized = true; };

	bool initialized;

	AccumulatorAndProofParams accumulatorParams;

	/**
	 * The Quadratic Residue group from which we form
	 * a coin as a commitment  to a serial number.
	 */
  IntegerGroupParams coinCommitmentGroup;

	/**
	 * One of two groups used to form a commitment to
	 * a coin (which it self is a commitment to a serial number).
	 * This is the one used in the serial number poof.
	 * It's order must be equal to the modulus of coinCommitmentGroup.
	 */
	IntegerGroupParams serialNumberSoKCommitmentGroup;

	/**
	 * The number of iterations to use in the serial
	 * number proof.
	 */
	uint32_t zkp_iterations;

	/**
	 * The amount of the hash function we use for
	 * proofs.
	 */
	uint32_t zkp_hash_len;

  /**
   * Maximum number of outputs and bitsize of amount
   * to prove in the rangeproof.
   */
  unsigned int maxNumberOutputs;
  unsigned int rangeProofBitSize;

  /**
   * Constraints tensors generated with ArithmeticCircuit::setPreConstraints()
   */
  std::vector<CBN_matrix> ZKP_wA;
  std::vector<CBN_matrix> ZKP_wB;
  std::vector<CBN_matrix> ZKP_wC;
  CBN_vector ZKP_K;
  std::vector< std::vector< std::pair<int, CBigNum> > > S_POLY_A1, S_POLY_A2;
  std::vector< std::vector< std::pair<int, CBigNum> > > S_POLY_B1, S_POLY_B2;
  std::vector< std::vector< std::pair<int, CBigNum> > > S_POLY_C1, S_POLY_C2;
	
	ADD_SERIALIZE_METHODS;
  template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
      READWRITE(initialized);
      READWRITE(accumulatorParams);
      READWRITE(coinCommitmentGroup);
      READWRITE(serialNumberSoKCommitmentGroup);
	    READWRITE(zkp_iterations);
	    READWRITE(zkp_hash_len);
	}
};

} /* namespace libzerocoin */

#endif /* PARAMS_H_ */
