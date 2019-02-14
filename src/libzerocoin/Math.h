/**
* @file       Math.h
*
* @brief      Diverse vector operations
*             Inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
*
* @author     alex v
* @date       December 2018
*
* @copyright  Copyright 2018 alex v
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2018 The NavCoin Core developers

#ifndef VECTORMATH_H
#define VECTORMATH_H

#include "bignum.h"

bool VectorExponent(CBigNum &out, CBigNum a, std::vector<CBigNum> a_exp, CBigNum b, std::vector<CBigNum> b_exp, CBigNum mod)
{
    if (a_exp.size() != b_exp.size())
        return false;

    out.Nullify();
    for (unsigned int i = 0; i < a_exp.size(); i++)
    {
        out = a.pow(a_exp[i]);
        out = b.pow(b_exp[i]);
    }
    return true;
}

/* Compute a custom vector-scalar commitment mod */
bool VectorVectorExponentMod(CBigNum &out, std::vector<CBigNum> a, std::vector<CBigNum> a_exp, std::vector<CBigNum> b, std::vector<CBigNum> b_exp, CBigNum mod)
{
    if (!(a.size() == b.size() && a_exp.size() == b_exp.size() && a.size() == b_exp.size()))
        return false;

    out.Nullify();
    for (unsigned int i = 0; i < a.size(); i++)
    {
        out = (out * a[i].pow_mod(a_exp[i], mod)) % mod;
        out = (out * b[i].pow_mod(b_exp[i], mod)) % mod;
    }
    return true;
}

bool VectorVectorExponentMod(CBigNum &out, std::vector<CBigNum> a, std::vector<CBigNum> a_exp, std::vector<CBigNum> b_exp, CBigNum mod)
{
    if (!(a.size() > (a_exp.size() + b_exp.size()) && a_exp.size() == b_exp.size()))
        return false;

    out.Nullify();
    for (unsigned int i = 0; i < a_exp.size(); i++)
    {
        out = (out * a[i].pow_mod(a_exp[i], mod)) % mod;
        out = (out * a[i+a_exp.size()].pow_mod(b_exp[i], mod)) % mod;
    }
    return true;
}


/* Given a scalar, construct a vector of powers */
bool VectorPowers(std::vector<CBigNum>& out, CBigNum x, unsigned int size)
{
    out.clear();
    out.resize(size);

    std::vector<CBigNum> temp(size);

    for (unsigned int i = 0; i < size; i++)
    {
        temp[i] = x.pow(i);
    }

    out = temp;

    return true;
}

/* Given two scalar arrays, construct the inner product */
bool InnerProduct(CBigNum& out, std::vector<CBigNum> a, std::vector<CBigNum> b, CBigNum mod)
{
    if (a.size() != b.size())
        return false;

    out.Nullify();
    for (unsigned int i = 0; i < a.size(); i++)
    {
        out = (out + a[i].mul_mod(b[i], mod)) % mod;
    }
    return true;
}

/* Given two scalar arrays, construct the Hadamard product */
bool Hadamard(std::vector<CBigNum>& out, std::vector<CBigNum> a, std::vector<CBigNum> b, CBigNum mod)
{
    if (a.size() != b.size())
        return false;

    out.clear();
    out.resize(a.size());

    std::vector<CBigNum> temp(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        temp[i] = a[i].mul_mod(b[i], mod);
    }

    out = temp;

    return true;
}

/* Add two vectors */
bool VectorAdd(std::vector<CBigNum>& out, std::vector<CBigNum> a, std::vector<CBigNum> b, CBigNum mod)
{
    if (a.size() != b.size())
        return false;

    out.clear();
    out.resize(a.size());

    std::vector<CBigNum> temp(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        temp[i] = (a[i] + b[i]) % mod;
    }

    out = temp;

    return true;
}

/* Subtract two vectors */
bool VectorSubtract(std::vector<CBigNum>& out, std::vector<CBigNum> a, std::vector<CBigNum> b)
{
    if (a.size() != b.size())
        return false;

    out.clear();
    out.resize(a.size());

    std::vector<CBigNum> temp(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        temp[i] = a[i] - b[i];
    }

    out = temp;

    return true;
}

/* Multiply a scalar and a vector */
bool VectorScalar(std::vector<CBigNum>& out, std::vector<CBigNum> a, CBigNum x, CBigNum mod)
{
    std::vector<CBigNum> temp(out.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        temp[i] = a[i].mul_mod(x, mod);
    }

    out = temp;

    return true;
}

/* Exponentiate a vector by a scalar */
bool VectorScalarExp(std::vector<CBigNum>& out, std::vector<CBigNum> a, CBigNum x, CBigNum mod)
{
    std::vector<CBigNum> temp(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        temp[i] = a[i].pow_mod(x, mod);
    }

    out = temp;

    return true;
}

/* Compute the slice of a scalar vector */
bool VectorSlice(std::vector<CBigNum>& out, std::vector<CBigNum> a, unsigned int start, unsigned int stop)
{
    if (start > a.size() || stop > a.size())
        return false;

    out.clear();
    out.resize(stop-start);

    std::vector<CBigNum> temp(a.size());

    for (unsigned int i = start; i < stop; i++)
    {
        out[i-start] = a[i];
    }

    out = temp;

    return true;
}
#endif // VECTORMATH_H
