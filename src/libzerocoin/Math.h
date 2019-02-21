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

struct MultiexpData {
  CBigNum base;
  CBigNum exp;

  MultiexpData() {}
  MultiexpData(const CBigNum &s, const CBigNum &p): base(s), exp(p) {}
};

CBigNum MultiExp(std::vector<MultiexpData> multiexp_data, CBigNum modulus)
{
    CBigNum result = 1;

    for (MultiexpData& it: multiexp_data)
    {
        std::cout << it.base.ToString(16).substr(0,8) << "^" << it.exp.ToString(16).substr(0,8) << std::endl;
        result = result.mul_mod(it.base.pow_mod(it.exp, modulus), modulus);
    }

    return result;
}

CBigNum VectorPowerSum(const CBigNum &x, size_t n)
{
    if (n < 0)
        throw std::runtime_error("VectorPowerSum(): n can't be negative");

    if (n == 0)
        return 0;

    if (n == 1)
        return 1;

    CBigNum prev = x;
    CBigNum out = 1;

    for (size_t i = 1; i < n; ++i)
    {
        if (i > 1)
            prev = prev * x;
        out = out + prev;
    }

    return out;
}


std::vector<CBigNum> VectorExponent(const std::vector<CBigNum>& a, const CBigNum& a_exp, CBigNum mod)
{
    std::vector<CBigNum> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = a[i].pow_mod(a_exp, mod);
    }

    return ret;
}

/* Compute a custom vector-scalar commitment mod */
CBigNum VectorExponent2Mod(const std::vector<CBigNum>& a, const std::vector<CBigNum>& a_exp, const std::vector<CBigNum>& b, const std::vector<CBigNum>& b_exp, CBigNum mod)
{
    if (!(a.size() == b.size() && a_exp.size() == b_exp.size() && a.size() == b_exp.size()))
        throw std::runtime_error("VectorExponent2Mod(): a, b, a_exp and b_exp should be of the same size");

    CBigNum ret = 1;

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret = ret.mul_mod(a[i].pow_mod(a_exp[i], mod), mod);
        ret = ret.mul_mod(b[i].pow_mod(b_exp[i], mod), mod);
    }
    return ret;
}

CBigNum VectorExponent2Mod(std::vector<CBigNum> a, std::vector<CBigNum> a_exp, std::vector<CBigNum> b_exp, CBigNum mod)
{
    if (!(a.size() > (a_exp.size() + b_exp.size()) && a_exp.size() == b_exp.size()))
        throw std::runtime_error("VectorExponent2Mod(): a_exp and b_exp should be of the same size");

    CBigNum ret = 1;

    for (unsigned int i = 0; i < a_exp.size(); i++)
    {
        ret = ret.mul_mod(a[i].pow_mod(a_exp[i], mod), mod);
        ret = ret.mul_mod(a[i+a_exp.size()].pow_mod(b_exp[i], mod), mod);
    }

    return ret;
}


/* Given a scalar, construct a vector of powers */
std::vector<CBigNum> VectorPowers(CBigNum x, unsigned int size)
{
    std::vector<CBigNum> temp(size);

    for (unsigned int i = 0; i < size; i++)
    {
        temp[i] = x.pow(i);
    }

    return temp;
}

/* Given two scalar arrays, construct the inner product */
CBigNum InnerProduct(const std::vector<CBigNum>& a, const std::vector<CBigNum>& b, CBigNum mod)
{
    if (a.size() != b.size())
        throw std::runtime_error("InnerProduct(): a and b should be of the same size");

    CBigNum ret;

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret = (ret + a[i].mul_mod(b[i], mod)) % mod;
    }

    return ret;
}

CBigNum InnerProduct(const std::vector<CBigNum>& a, const std::vector<CBigNum>& b)
{
    if (a.size() != b.size())
        throw std::runtime_error("InnerProduct(): a and b should be of the same size");

    CBigNum ret;

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret = ret + a[i] * b[i];
    }

    return ret;
}

/* Given two scalar arrays, construct the Hadamard product */
std::vector<CBigNum> Hadamard(const std::vector<CBigNum>& a, const std::vector<CBigNum>& b, CBigNum mod)
{
    if (a.size() != b.size())
        throw std::runtime_error("Hadamard(): a and b should be of the same size");

    std::vector<CBigNum> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = a[i].mul_mod(b[i], mod);
    }

    return ret;
}

/* Add two vectors */
std::vector<CBigNum> VectorAdd(const std::vector<CBigNum>& a, const std::vector<CBigNum>& b, CBigNum mod)
{
    if (a.size() != b.size())
        throw std::runtime_error("VectorAdd(): a and b should be of the same size");

    std::vector<CBigNum> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = (a[i] + b[i]) % mod;
    }

    return ret;
}

std::vector<CBigNum> VectorAdd(const std::vector<CBigNum>& a, const CBigNum& b)
{
    std::vector<CBigNum> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = a[i] + b;
    }

    return ret;
}

/* Subtract two vectors */
std::vector<CBigNum> VectorSubtract(const std::vector<CBigNum>& a, const CBigNum& b)
{

    std::vector<CBigNum> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = a[i] - b;
    }

    return ret;
}

/* Multiply a scalar and a vector */
std::vector<CBigNum> VectorScalar(const std::vector<CBigNum>& a, const CBigNum& x, CBigNum mod)
{
    std::vector<CBigNum> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = a[i].mul_mod(x, mod);
    }

    return ret;
}

/* Compute the slice of a scalar vector */
std::vector<CBigNum> VectorSlice(const std::vector<CBigNum>& a, unsigned int start, unsigned int stop)
{
    if (start > a.size() || stop > a.size())
        throw std::runtime_error("VectorSlice(): wrong start or stop point");

    std::vector<CBigNum> ret(stop-start);

    for (unsigned int i = start; i < stop; i++)
    {
        ret[i-start] = a[i];
    }

    return ret;
}

CBigNum CrossVectorExponent(size_t size, const std::vector<CBigNum> &A, size_t Ao, const std::vector<CBigNum> &B, size_t Bo, const std::vector<CBigNum> &a, size_t ao, const std::vector<CBigNum> &b, size_t bo, const std::vector<CBigNum> *scale, const CBigNum *extra_point, const CBigNum *extra_scalar, const CBigNum modulus)
{
//  CHECK_AND_ASSERT_THROW_MES(size + Ao <= A.size(), "Incompatible size for A");
//  CHECK_AND_ASSERT_THROW_MES(size + Bo <= B.size(), "Incompatible size for B");
//  CHECK_AND_ASSERT_THROW_MES(size + ao <= a.size(), "Incompatible size for a");
//  CHECK_AND_ASSERT_THROW_MES(size + bo <= b.size(), "Incompatible size for b");
//  CHECK_AND_ASSERT_THROW_MES(size <= maxN*maxM, "size is too large");
//  CHECK_AND_ASSERT_THROW_MES(!scale || size == scale->size() / 2, "Incompatible size for scale");
//  CHECK_AND_ASSERT_THROW_MES(!!extra_point == !!extra_scalar, "only one of extra point/scalar present");

    std::vector<MultiexpData> multiexp_data;
    multiexp_data.resize(size*2 + (!!extra_point));
    for (size_t i = 0; i < size; ++i)
    {
        multiexp_data[i*2].exp = a[ao+i];
        multiexp_data[i*2].base = A[Ao+i];
        multiexp_data[i*2+1].exp = b[bo+i];

        if (scale)
            multiexp_data[i*2+1].exp =  multiexp_data[i*2+1].exp * (*scale)[Bo+i];

        multiexp_data[i*2+1].base = B[Bo+i];
    }
    if (extra_point)
    {
        multiexp_data.back().exp = *extra_scalar;
        multiexp_data.back().base = *extra_point;
    }

    return MultiExp(multiexp_data, modulus).pow_mod(-1, modulus);
}

std::vector<CBigNum> HadamardFold(std::vector<CBigNum> &vec, std::vector<CBigNum> *scale, const CBigNum &a, const CBigNum &b, const CBigNum& mod, const CBigNum& order)
{
  if(!((vec.size() & 1) == 0))
      throw std::runtime_error("HadamardFold(): vector argument size is not even");

  const size_t sz = vec.size() / 2;
  std::vector<CBigNum> out(sz);

  for (size_t n = 0; n < sz; ++n)
  {
    CBigNum c0 = vec[n];
    CBigNum c1 = vec[sz + n];
    CBigNum sa, sb;
    if (scale) sa = a.mul_mod((*scale)[n], order); else sa = a;
    if (scale) sb = b.mul_mod((*scale)[sz + n], order); else sb = b;
    out[n] = c0.pow_mod(sa, mod).mul_mod(c1.pow_mod(sb, mod), mod);
  }

  return out;
}

#endif // VECTORMATH_H
