/**
* @file       zkplib.h
*
* @brief      Auxiliary functions for the Zerocoin library.
*
* @author     Mary Maller, Jonathan Bootle and Gian Piero Dionisio
* @date       April 2018
*
* @copyright  Copyright 2018 The PIVX Developers
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2019 The NavCoin Core developers

#pragma once

namespace libzerocoin {


inline void vectorTimesConstant(CBN_vector& kV,
        CBN_vector& V, const CBigNum& k, const CBigNum& modulus)
{
    transform(V.begin(), V.end(), kV.begin(),
            [=] (CBigNum Vi) {
        return Vi.mul_mod(k,modulus);} );
}

inline CBN_vector vectorTimesConstant(
        CBN_vector& V, const CBigNum& k, const CBigNum& modulus)
{
    CBN_vector kV(V.size());
    vectorTimesConstant(kV, V, k, modulus);
    return kV;
}

inline void addVectors_mod(CBN_vector& sum,
        CBN_vector& v1, CBN_vector& v2, const CBigNum& modulus)
{
    if(v1.size() != v2.size())
        throw std::runtime_error("different vector length in addVectors_mod");

    sum.resize(v1.size());

    transform(v1.begin(), v1.end(), v2.begin(), sum.begin(),
            [=] (CBigNum v1_i, CBigNum v2_i) {
        return (v1_i + v2_i) % modulus;} );
}

inline void unit_vector(CBN_vector& v, const unsigned int j)
{
    std::fill(v.begin(), v.end(), CBigNum(0));
    v[j] = CBigNum(1);
}

inline CBigNum dotProduct(const CBN_vector& u, const CBN_vector& v,
        const CBigNum& modulus, const unsigned int size)
{
    CBigNum dot = CBigNum(0);

    for(unsigned int i=0; i<size; i++)
        dot = (dot + u[i].mul_mod(v[i], modulus)) % modulus;

    return dot;
}

inline CBigNum dotProduct(const CBN_vector& u, const CBN_vector& v, const CBigNum& modulus)
{
    if(u.size() != v.size())
        throw std::runtime_error("different vector length in dotProduct");

    return dotProduct(u, v, modulus, u.size());
}

inline void random_vector_mod(CBN_vector& v, const CBigNum& modulus)
{
    for(unsigned int i=0; i<v.size(); i++)
        v[i] = CBigNum::randBignum(modulus);
}

inline CBigNum pedersenCommitment(const IntegerGroupParams* IGparams,
        const CBN_vector& g_blinders, const CBigNum& h_blinder)
{
    const CBigNum p = IGparams->modulus;

    // assert len(gelements) >= len(g_blinders)
    if( IGparams->gis.size() < g_blinders.size() )
        throw std::runtime_error("len(gelements) < len(g_blinders) in pedersenCommit");

    CBigNum C = CBigNum(1);
    for(unsigned int i=0; i<g_blinders.size(); i++)
        C = C.mul_mod((IGparams->gis[i]).pow_mod(g_blinders[i],p),p);
    C = C.mul_mod((IGparams->h).pow_mod(h_blinder,p),p);

    return C;
}
/*
// returns bitvector of least significant byte
inline void binary_lookup(std::vector<int>& bits, const int i)
{
    if(bits.size()) bits.clear();
    for(unsigned int pos=0; pos<8; pos++)
        bits.push_back(i >> pos & 1);
}
*/
// Initialize sets for inner product
inline std::pair<CBN_matrix, CBN_matrix> ck_inner_gen(
        const IntegerGroupParams* IGp, const CBigNum& y)
{
    const CBigNum q = IGp->groupOrder;
    const CBigNum p = IGp->modulus;
    CBN_matrix ck_inner_g(1, CBN_vector());
    CBN_matrix ck_inner_h(1, CBN_vector());

    CBigNum exp = CBigNum(1);
    CBigNum ym = y.pow_mod(-ZKP_M, q);

    for(int j=0; j<(ZKP_N+ZKP_PADS); j++) {
        ck_inner_g[0].push_back(IGp->gis[j]);
        exp = exp.mul_mod(ym,q);
        ck_inner_h[0].push_back(IGp->gis[j].pow_mod(exp,p));
    }

    return make_pair(ck_inner_g, ck_inner_h);
}

// Initialize sets for inner product - for batching
inline CBN_matrix ck_inner_gen(const IntegerGroupParams* IGp)
{
    CBN_matrix ck_inner_g(1, CBN_vector());

    for(int j=0; j<(ZKP_N+ZKP_PADS); j++)
        ck_inner_g[0].push_back(IGp->gis[j]);

    return ck_inner_g;
}


inline void hadamard(CBN_vector& had,
        CBN_vector& u, CBN_vector& v, const CBigNum& modulus)
{
    if(u.size() != v.size())
        throw std::runtime_error("different vector length in hadamard");


    had.resize(u.size());

    transform(u.begin(), u.end(), v.begin(), had.begin(),
            [=] (CBigNum u_i, CBigNum v_i) {
        return u_i.mul_mod(v_i, modulus);} );
}

// Print Functions
inline void printVector(const CBN_vector v)
{
    std::cout << "[";
    for(unsigned int i=0; i<v.size()-1; i++)
        std::cout << v[i] << ",  ";
    std::cout << v[v.size()-1] << "]";

}

inline void printMatrix(const CBN_matrix w)
{
    std::cout << "[";
    for(unsigned int i=0; i<w.size()-1; i++) {
        printVector(w[i]);
        std::cout << ",  ";
    }
    printVector(w[w.size()-1]);
    std::cout << "]";
}

inline CBigNum XorObfuscate(std::string pid, CBigNum key, unsigned int len) {
    std::string truncatedPid = pid.substr(0,len);
    std::vector<unsigned char> vPid;

    vPid.push_back(truncatedPid.length());
    vPid.insert(vPid.end(), truncatedPid.begin(), truncatedPid.end());

    CBigNum bnRand = CBigNum::RandKBitBigum((len-vPid.size())*8);
    std::vector<unsigned char> vRand = bnRand.getvch();

    vRand.resize(32-vPid.size());
    vPid.insert(vPid.end(), vRand.begin(), vRand.end());

    CBigNum bnPid;

    bnPid.setvch(vPid);

    return bnPid.Xor(key);
}

} /* namespace libzerocoin */
