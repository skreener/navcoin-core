/**
* @file       Math.h
*
* @brief      Bulletproofs Rangeproof
*             Inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
*
* @author     alex v
* @date       December 2018
*
* @copyright  Copyright 2018 alex v
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2018-2019 The NavCoin Core developers

#include "bulletproof_rangeproof.h"

bool BulletproofRangeproof::Prove(std::vector<CBigNum> v, std::vector<CBigNum> gamma, unsigned int logM)
{
    if (v.size() != gamma.size())
        return false;

    unsigned int M = v.size();

    int logMN = logM + this->nexp;

    if (pow(2, logM) < v.size())
        return false;

    // V is a vector with commitments in the form g^v h^gamma

    this->V.resize(M);
    this->V[0] = p->g.pow_mod(v[0], p->modulus).mul_mod(p->h.pow_mod(gamma[0], p->modulus), p->modulus);

    // This hash is updated for Fiat-Shamir throughout the proof

    CHashWriter hasher(0,0);

    hasher << this->V[0];

    for (unsigned int j = 1; j < M; j++)
    {
        this->V[j] = p->g.pow_mod(v[j], p->modulus).mul_mod(p->h.pow_mod(gamma[j], p->modulus), p->modulus);
        hasher << this->V[j];
    }

    // PAPER LINES 41-42
    // Value to be obfuscated is encoded in binary in aL
    // aR is aL-1

    std::vector<CBigNum> aL(M*this->n);
    std::vector<CBigNum> aR(M*this->n);

    for (unsigned int j = 0; j < M; j++)
    {
        CBigNum tempV = v[j];

        for (int i = this->n-1; i>=0; i--)
        {
            CBigNum basePow = CBigNum(2).pow(i);

            if(tempV.div(basePow) == CBigNum())
            {
                aL[j*this->n+i] = CBigNum();
            }
            else
            {
                aL[j*this->n+i] = CBigNum(1);
                tempV = tempV - basePow;
            }

            aR[j*this->n+i] = aL[j*this->n+i] - CBigNum(1);
        }
    }

    // PAPER LINES 43-44
    // Commitment to aL and aR (obfuscated with alpha)

    CBigNum alpha = CBigNum::randBignum(p->groupOrder);

    if(!VectorVectorExponentMod(this->A, p->gis, aL, aR, p->modulus))
        return false;

    this->A = this->A.mul_mod(p->g.pow_mod(alpha, p->modulus), p->modulus);

    // PAPER LINES 45-47
    // Commitment to blinding sL and sR (obfuscated with rho)

    std::vector<CBigNum> sL(M*this->n);
    std::vector<CBigNum> sR(M*this->n);

    for (unsigned int i = 0; i < M*this->n; i++)
    {
        sL[i] = CBigNum::randBignum(p->groupOrder);
        sR[i] = CBigNum::randBignum(p->groupOrder);
    }

    CBigNum rho = CBigNum::randBignum(p->groupOrder);

    if(!VectorVectorExponentMod(this->S, p->gis, sL, sR, p->modulus))
        return false;

    this->S = this->S.mul_mod(p->g.pow_mod(rho, p->modulus), p->modulus);

    // PAPER LINES 48-50

    hasher << this->A;
    hasher << this->S;

    CBigNum y = CBigNum(hasher.GetHash());

    std::cout << "y " << y.ToString(16) << std::endl;

    hasher << y;

    CBigNum z = CBigNum(hasher.GetHash());

    std::cout << "z " << z.ToString(16) << std::endl;

    // Polynomial construction by coefficients
    // PAPER LINE AFTER 50

    std::vector<CBigNum> l0;
    std::vector<CBigNum> l1;
    std::vector<CBigNum> r0;
    std::vector<CBigNum> r1;

    std::vector<CBigNum> vTempBN;

    // l(x) = (aL - z 1^n) + sL X

    if(!VectorPowers(vTempBN, CBigNum(1), M*this->n))
        return false;

    if(!VectorScalarExp(vTempBN, vTempBN, z, p->groupOrder))
        return false;

    if(!VectorSubtract(l0, aL, vTempBN))
        return false;

    // l(1) is (aL - z 1^n) + sL, but this is not used so we keep it to sL

    l1 = sL;

    // This computes the ugly sum/concatenation from page 10
    // Calculation of r(0) and r(1)

    std::vector<CBigNum> zerosTwos(M*this->n);
    for(unsigned int i = 0; i<M*this->n; i++)
    {
        zerosTwos[i] = 0;

        for (unsigned int j = 1; j<=M; j++) // note this starts from 1
        {
            CBigNum tempBN = 0;

            if (i >= (j-1)*this->n && i < j*this->n)
                tempBN = CBigNum(2).pow(i-(j-1)*this->n); // exponent ranges from 0..N-1

            zerosTwos[i] = (zerosTwos[i] + z.pow(1+j))*(tempBN);
        }
    }

    if (!VectorPowers(vTempBN, CBigNum(1), M*this->n))
        return false;

    if (!VectorScalar(vTempBN, vTempBN, z, p->groupOrder))
        return false;

    if (!VectorAdd(r0, aR, vTempBN, p->groupOrder))
        return false;

    if (!VectorPowers(vTempBN, y, M*this->n))
        return false;

    if (!Hadamard(r0, r0, vTempBN, p->groupOrder))
        return false;

    if (!VectorAdd(r0, r0, zerosTwos, p->groupOrder))
        return false;

    if (!VectorPowers(vTempBN, y, M*this->n))
        return false;

    if (!Hadamard(r1, vTempBN, sR, p->groupOrder))
        return false;

    // Polynomial construction before PAPER LINE 51
    CBigNum t0;
    CBigNum t1;
    CBigNum t2;
    CBigNum tempBN;
    CBigNum tempBN2;

    if (!InnerProduct(t0, l0, r0, p->groupOrder))
        return false;

    if (!InnerProduct(tempBN, l0, r1, p->groupOrder))
        return false;

    if (!InnerProduct(tempBN2, l1, r0, p->groupOrder))
        return false;

    t1 = tempBN + tempBN2;

    if (!InnerProduct(t2, l1, r1, p->groupOrder))
        return false;

    // PAPER LINES 52-53
    CBigNum tau1 = CBigNum::randBignum(p->groupOrder);
    CBigNum tau2 = CBigNum::randBignum(p->groupOrder);

    this->T1 = p->h.pow_mod(t1, p->modulus).mul_mod(p->g.pow_mod(tau1, p->modulus), p->modulus);
    this->T2 = p->h.pow_mod(t2, p->modulus).mul_mod(p->g.pow_mod(tau2, p->modulus), p->modulus);

    // PAPER LINES 54-56
    hasher << z;
    hasher << this->T1;
    hasher << this->T2;

    CBigNum x = CBigNum(hasher.GetHash());

    std::cout << "x " << x.ToString(16) << std::endl;

    // PAPER LINES 58-59
    std::vector<CBigNum> l = l0;

    if (!VectorScalar(vTempBN, l1, x, p->groupOrder))
        return false;

    if (!VectorAdd(l, l, vTempBN, p->groupOrder))
        return false;

    std::vector<CBigNum> r = r0;

    if (!VectorScalar(vTempBN, r1, x, p->groupOrder))
        return false;

    if (!VectorAdd(r, r, vTempBN, p->groupOrder))
        return false;

    // PAPER LINE 60

    if (!InnerProduct(this->t, l, r, p->groupOrder))
        return false;

    // PAPER LINES 61-62
    this->taux = tau1 * x;
    this->taux = this->taux + (tau2 * x.pow(2));

    for (unsigned int j = 1; j <= M; j++) // note this starts from 1
    {
        this->taux = this->taux + (z.pow(1+j)*(gamma[j-1]));
    }

    this->mu = (x * rho) + alpha;

    // PAPER LINES 32-33
    hasher << x;
    hasher << this->taux;
    hasher << this->mu;
    hasher << this->t;

    CBigNum x_ip = CBigNum(hasher.GetHash());

    std::cout << "x_ip " << x_ip.ToString(16) << std::endl;

    // These are used in the inner product rounds
    unsigned int nprime = M*this->n;

    std::vector<CBigNum> gprime(nprime);
    std::vector<CBigNum> hprime(nprime);
    std::vector<CBigNum> aprime(nprime);
    std::vector<CBigNum> bprime(nprime);

    for (unsigned int i = 0; i < nprime; i++)
    {
        gprime[i] = p->gis[i];
        hprime[i] = p->gis[i+nprime].pow_mod(y.pow(i).inverse(p->groupOrder), p->modulus);
        aprime[i] = l[i];
        bprime[i] = r[i];
    }

    this->L.resize(logMN);
    this->R.resize(logMN);

    unsigned int round = 0;

    std::vector<CBigNum> w(logMN);

    while (nprime > 1)
    {
        // PAPER LINE 20

        nprime /= 2;

        std::vector<CBigNum> vTempBN;
        std::vector<CBigNum> vTempBN2;
        std::vector<CBigNum> vTempBN3;
        std::vector<CBigNum> vTempBN4;

        // PAPER LINES 21-22

        if (!VectorSlice(vTempBN, aprime, 0, nprime))
            return false;

        if (!VectorSlice(vTempBN2, bprime, nprime, bprime.size()))
            return false;

        CBigNum cL;

        if (!InnerProduct(cL, vTempBN, vTempBN2, p->groupOrder))
            return false;

        if (!VectorSlice(vTempBN, aprime, nprime, aprime.size()))
            return false;

        if (!VectorSlice(vTempBN2, bprime, 0, nprime))
            return false;

        CBigNum cR;

        if (!InnerProduct(cR, vTempBN, vTempBN2, p->groupOrder))
            return false;

        // PAPER LINES 23-24

        if (!VectorSlice(vTempBN, gprime, nprime, gprime.size()))
            return false;

        if (!VectorSlice(vTempBN2, hprime, 0, nprime))
            return false;

        if (!VectorSlice(vTempBN3, aprime, 0, nprime))
            return false;

        if (!VectorSlice(vTempBN4, bprime, nprime, bprime.size()))
            return false;

        if (!VectorVectorExponentMod(this->L[round], vTempBN, vTempBN3, vTempBN2, vTempBN4, p->modulus))
            return false;

        this->L[round] = this->L[round] * p->h.pow_mod(cL*x_ip,p->modulus);

        if (!VectorSlice(vTempBN, gprime, 0, nprime))
            return false;

        if (!VectorSlice(vTempBN2, hprime, nprime, hprime.size()))
            return false;

        if (!VectorSlice(vTempBN3, aprime, nprime, aprime.size()))
            return false;

        if (!VectorSlice(vTempBN4, bprime, 0, nprime))
            return false;

        if (!VectorVectorExponentMod(this->R[round], vTempBN, vTempBN3, vTempBN2, vTempBN4, p->modulus))
            return false;

        this->R[round] = this->R[round].mul_mod(p->h.pow_mod(cR.mul_mod(x_ip, p->groupOrder), p->modulus), p->modulus);

        // PAPER LINES 25-27

        hasher << this->L[round];
        hasher << this->R[round];

        w[round] = CBigNum(hasher.GetHash());

        std::cout << "w["<< round <<"] " << w[round].ToString(16) << std::endl;

        // PAPER LINES 29-31

        if (!VectorSlice(vTempBN, gprime, 0, nprime))
            return false;

        if (!VectorScalar(vTempBN2, vTempBN, w[round].inverse(p->groupOrder), p->modulus))
            return false;

        if (!VectorSlice(vTempBN3, gprime, nprime, gprime.size()))
            return false;

        if (!VectorScalar(vTempBN4, vTempBN3, w[round], p->modulus))
            return false;

        if (!Hadamard(gprime, vTempBN2, vTempBN4, p->modulus))
            return false;

        if (!VectorSlice(vTempBN, hprime, 0, nprime))
            return false;

        if (!VectorScalar(vTempBN2, vTempBN, w[round], p->modulus))
            return false;

        if (!VectorSlice(vTempBN3, hprime, nprime, hprime.size()))
            return false;

        if (!VectorScalar(vTempBN4, vTempBN3, w[round].inverse(p->groupOrder), p->modulus))
            return false;

        if (!Hadamard(hprime, vTempBN2, vTempBN4, p->modulus))
            return false;

        // PAPER LINES 33-34
        if (!VectorSlice(vTempBN, aprime, 0, nprime))
            return false;

        if (!VectorScalar(vTempBN2, vTempBN, w[round], p->groupOrder))
            return false;

        if (!VectorSlice(vTempBN3, aprime, nprime, aprime.size()))
            return false;

        if (!VectorScalar(vTempBN4, vTempBN3, w[round].inverse(p->groupOrder), p->groupOrder))
            return false;

        if (!VectorAdd(aprime, vTempBN2, vTempBN4, p->groupOrder))
            return false;

        if (!VectorSlice(vTempBN, bprime, 0, nprime ))
            return false;

        if (!VectorScalar(vTempBN2, vTempBN, w[round].inverse(p->groupOrder), p->groupOrder))
            return false;

        if (!VectorSlice(vTempBN3, bprime, nprime, bprime.size()))
            return false;

        if (!VectorScalar(vTempBN4, vTempBN3, w[round], p->groupOrder))
            return false;

        if (!VectorAdd(bprime, vTempBN2, vTempBN4, p->groupOrder))
            return false;

        round += 1;
    }

    this->R.resize(round);
    this->L.resize(round);

    this->a = aprime[0];
    this->b = bprime[0];

    return true;

}

bool BulletproofRangeproof::Verify()
{
    unsigned int maxMN = pow(2, L.size());

    CBigNum y0 = 0; // tau_x
    CBigNum y1 = 0; // t-(k+z+Sum(y^i))
    CBigNum Y2 = 0;
    CBigNum Y3 = 0;
    CBigNum Y4 = 0;

    CBigNum Z0 = 0;
    CBigNum z1 = 0;
    CBigNum Z2 = 0;
    CBigNum z3 = 0;
    std::vector<CBigNum> z4(maxMN);
    std::vector<CBigNum> z5(maxMN);

    for (unsigned int i = 0; i < maxMN; i++)
    {
        z4[i] = 0;
        z5[i] = 0;
    }

    unsigned int logMN = L.size();
    unsigned int M = pow(2,logMN)/this->n;

    CBigNum weight = CBigNum::randBignum(p->groupOrder);

    CHashWriter hasher(0,0);

    hasher << this->V[0];

    for (unsigned int j = 1; j < this->V.size(); j++)
        hasher << this->V[j];

    hasher << A;
    hasher << S;

    CBigNum y = CBigNum(hasher.GetHash());

    hasher << y;

    CBigNum z = CBigNum(hasher.GetHash());

    hasher << z;
    hasher << T1;
    hasher << T2;

    CBigNum x = CBigNum(hasher.GetHash());

    hasher << x;
    hasher << taux;
    hasher << mu;
    hasher << t;

    CBigNum x_ip = CBigNum(hasher.GetHash());

    std::cout << "y " << y.ToString(16) << std::endl;
    std::cout << "z " << z.ToString(16) << std::endl;
    std::cout << "x " << x.ToString(16) << std::endl;
    std::cout << "x_ip " << x_ip.ToString(16) << std::endl;

    // PAPER LINE 65

    y0 = y0 + taux * weight;

    CBN_vector vBNTemp1;
    CBN_vector vBNTemp2;

    CBigNum bnTemp;

    if (!VectorPowers(vBNTemp1, 1, M*this->n))
        return false;

    if (!VectorPowers(vBNTemp2, y, M*this->n))
        return false;

    if (!InnerProduct(bnTemp, vBNTemp1, vBNTemp2, p->groupOrder))
        return false;

    CBigNum k = 0 - z.pow(2)*(bnTemp);

    for (unsigned int j = 1; j <= M; j++) // note this starts from 1
    {
        if (!VectorPowers(vBNTemp1, 1, this->n))
            return false;

        if (!VectorPowers(vBNTemp2, 2, this->n))
            return false;

        if (!InnerProduct(bnTemp, vBNTemp1, vBNTemp2, p->groupOrder))
            return false;

        k = k - z.pow(j+2)*(bnTemp);

    }

    if (!VectorPowers(vBNTemp1, 1, M*this->n))
        return false;

    if (!VectorPowers(vBNTemp2, y, M*this->n))
        return false;

    if (!InnerProduct(bnTemp, vBNTemp1, vBNTemp2, p->groupOrder))
        return false;

    y1 = y1 + (t - k + z * (bnTemp)) * weight;

    bnTemp = 0;

    for (unsigned int j = 0; j < this->V.size(); j++)
    {
        bnTemp = bnTemp + (this->V[j].pow_mod(z.pow(j+2), p->modulus));
    }

    Y2 = (Y2 + bnTemp.pow_mod(weight, p->modulus)) % p->modulus;
    Y3 = (Y3 + T1.pow_mod(x * weight, p->modulus)) % p->modulus;
    Y4 = (Y4 + T2.pow_mod(x * x * weight, p->modulus)) % p->modulus;

    // PAPER LINE 66

    Z0 = (Z0 + ((A + (S.pow_mod(x, p->modulus))).pow_mod(weight, p->modulus))) % p->modulus;

    // PAPER LINES 25-27
    // The inner product challenges are computed per round
    CBN_vector w = CBN_vector(L.size());

    hasher << L[0];
    hasher << R[0];

    w[0] = CBigNum(hasher.GetHash());
    std::cout << "w["<< 0 <<"] " << w[0].ToString(16) << std::endl;

    if (logMN > 1)
    {
        for (unsigned int j = 1; j < logMN; j++)
        {
            hasher << L[j];
            hasher << R[j];

            w[j] = CBigNum(hasher.GetHash());
            std::cout << "w["<< j <<"] " << w[j].ToString(16) << std::endl;
        }
    }

    // Basically PAPER LINES 28-30
    // Compute the curvepoints from G[i] and H[i]

    for (unsigned int j = 0; j < M*this->n; j++)
    {
        // Convert the index to binary IN REVERSE and construct the scalar exponent
        unsigned int index = j;
        CBigNum gScalar = a;
        CBigNum hScalar = b * (y.pow(j).inverse(p->groupOrder));

        for (unsigned int jj = logMN; jj > 0; jj--)
        {
            unsigned int J = w.size() - jj; // because this is done in reverse bit order
            unsigned int basePow = pow(2, jj - 1); // assumes we don't get too big
            if (index / basePow == 0) // bit is zero
            {
                gScalar = gScalar.mul_mod(w[J].inverse(p->groupOrder), p->groupOrder);
                hScalar = hScalar.mul_mod(w[J], p->groupOrder);
            }
            else // bit is one
            {
                gScalar = gScalar.mul_mod(w[J], p->groupOrder);
                hScalar = hScalar.mul_mod(w[J].inverse(p->groupOrder), p->groupOrder);
                index -= basePow;
            }
        }

        gScalar = gScalar + z;
        hScalar = hScalar - (z.mul_mod(y.pow(j), p->groupOrder) + (z.pow(2+j/this->n)*(CBigNum(2).pow(j%this->n)))*(y.pow(j).inverse(p->groupOrder)));

        // Now compute the basepoint's scalar multiplication
        z4[j] = z4[j] + (gScalar * (weight));
        z5[j] = z5[j] + (hScalar * (weight));
    }

    // PAPER LINE 31
    z1 = z1 + (mu * (weight));

    bnTemp = 0;
    for (unsigned int j = 0; j < logMN; j++)
    {
        bnTemp = bnTemp + (L[j].pow_mod(w[j].pow(2), p->modulus));
        bnTemp = bnTemp + (R[j].pow_mod(w[j].pow(2).inverse(p->groupOrder), p->modulus));
    }
    Z2 = Z2 + (bnTemp.pow_mod(weight, p->modulus));
    z3 = z3 + ((t - (a * b)) * (x_ip) * (weight));

    // Perform the first- and second-stage check on all proofs at once
    CBigNum Check1 = (p->h.pow_mod(y0, p->modulus));
    Check1 = Check1.mul_mod(p->g.pow_mod(y0, p->modulus), p->modulus);
    Check1 = Check1.mul_mod(p->g.pow_mod(y1, p->modulus), p->modulus);
    Check1 = Check1.mul_mod(Y2.pow_mod(-1, p->modulus), p->modulus);
    Check1 = Check1.mul_mod(Y3.pow_mod(-1, p->modulus), p->modulus);
    Check1 = Check1.mul_mod(Y4.pow_mod(-1, p->modulus), p->modulus);

    if (Check1 != 0)
    {
        std::cout << "Failed first-stage check" << std::endl;
        return false;
    }

    CBigNum Check2 = Z0;
    Check2 = Check2.mul_mod(p->g.pow_mod(0-z1, p->modulus), p->modulus);
    Check2 = Check2.mul_mod(Z2, p->modulus);
    Check2 = Check2.mul_mod(p->h.pow_mod(z3, p->modulus), p->modulus);

    for (unsigned int i = 0; i < maxMN; i++)
    {
        Check2 = Check2.mul_mod(p->gis[i].pow_mod(0 - z4[i], p->modulus), p->modulus);
        Check2 = Check2.mul_mod(p->gis[i+maxMN].pow_mod(0 - z5[i], p->modulus), p->modulus);
    }

    if (Check2 != 0)
    {
        std::cout << "Failed second-stage check" << std::endl;
        return false;
    }

    return true;

}


