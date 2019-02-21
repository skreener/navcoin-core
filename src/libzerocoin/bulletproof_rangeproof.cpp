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

CBN_vector oneN = VectorPowers(1, BulletproofRangeproof::maxN);
CBN_vector twoN = VectorPowers(2, BulletproofRangeproof::maxN);
CBigNum ip12 = InnerProduct(oneN, twoN);

void BulletproofRangeproof::Prove(CBN_vector v, CBN_vector gamma)
{
    if (v.size() != gamma.size() || v.empty())
        throw std::runtime_error("BulletproofRangeproof::Prove(): Invalid vector size");

    const size_t logN = 6;
    const size_t N = 1<<logN;

    CBigNum p = params->modulus;
    CBigNum q = params->groupOrder;

    size_t M, logM;
    for (logM = 0; (M = 1<<logM) <= maxM && M < v.size(); ++logM);
    const size_t logMN = logM + logN;
    const size_t MN = M * N;

    // V is a vector with commitments in the form g^v h^gamma
    this->V.resize(v.size());

    // This hash is updated for Fiat-Shamir throughout the proof
    CHashWriter hasher(0,0);

    for (unsigned int j = 0; j < M; j++)
    {
        this->V[j] = params->g.pow_mod(v[j], p).mul_mod(params->h.pow_mod(gamma[j], p), p).pow_mod(-1, p);
        hasher << this->V[j];
    }

    // PAPER LINES 41-42
    // Value to be obfuscated is encoded in binary in aL
    // aR is aL-1
    CBN_vector aL(MN), aR(MN);

    for (unsigned int j = 0; j < M; j++)
    {
        CBigNum tempV = v[j];

        for (int i = N-1; i>=0; i--)
        {

            CBigNum basePow = CBigNum(2).pow(i);

            if(tempV.div(basePow) == CBigNum())
            {
                aL[j*N+i] = CBigNum();
            }
            else
            {
                aL[j*N+i] = CBigNum(1);
                tempV = tempV - basePow;
            }

            aR[j*N+i] = aL[j*N+i] - CBigNum(1);
        }
    }

try_again:
    // PAPER LINES 43-44
    // Commitment to aL and aR (obfuscated with alpha)
    CBigNum alpha = CBigNum::randBignum(q);

    this->A = VectorExponent2Mod(params->gis, aL, aR, p);
    this->A = this->A.mul_mod(params->h.pow_mod(alpha, p), p).pow_mod(-1, p);

    // PAPER LINES 45-47
    // Commitment to blinding sL and sR (obfuscated with rho)
    CBN_vector sL(MN);
    CBN_vector sR(MN);

    for (unsigned int i = 0; i < MN; i++)
    {
        sL[i] = CBigNum::randBignum(q);
        sR[i] = CBigNum::randBignum(q);
    }

    CBigNum rho = CBigNum::randBignum(q);

    this->S = VectorExponent2Mod(params->gis, sL, sR, p);
    this->S = this->S.mul_mod(params->h.pow_mod(rho, p), p).pow_mod(-1, p);

    // PAPER LINES 48-50
    hasher << this->A;
    hasher << this->S;

    CBigNum y = CBigNum(hasher.GetHash());

    if (y == 0)
        goto try_again;

    hasher << y;

    CBigNum z = CBigNum(hasher.GetHash());

    if (z == 0)
        goto try_again;

    // Polynomial construction by coefficients
    // PAPER LINE AFTER 50
    CBN_vector l0;
    CBN_vector l1;
    CBN_vector r0;
    CBN_vector r1;

    // l(x) = (aL - z 1^n) + sL X
    l0 = VectorSubtract(aL, z);

    // l(1) is (aL - z 1^n) + sL, but this is reduced to sL
    l1 = sL;

    // This computes the ugly sum/concatenation from page 19
    // Calculation of r(0) and r(1)
    CBN_vector zerosTwos(MN);
    CBN_vector zpow(M+2);

    zpow = VectorPowers(z, M+2);

    for(unsigned int i = 0; i<MN; i++)
    {
        zerosTwos[i] = 0;

        for (unsigned int j = 1; j<=M; j++) // note this starts from 1
        {
            if (i >= (j-1)*N && i < j*N)
                zerosTwos[i] = zerosTwos[i] + (zpow[1+j]*twoN[i-(j-1)*N]);
        }
    }

    CBN_vector yMN = VectorPowers(y, MN);
    r0 = VectorAdd(Hadamard(VectorAdd(aR, z), yMN, q), zerosTwos, q);
    r1 = Hadamard(yMN, sR, q);

    // Polynomial construction before PAPER LINE 51
    CBigNum t1 = ((InnerProduct(l0, r1, q) + InnerProduct(l1, r0, q)) % q);
    CBigNum t2 = InnerProduct(l1, r1, q);

    // PAPER LINES 52-53
    CBigNum tau1 = CBigNum::randBignum(q);
    CBigNum tau2 = CBigNum::randBignum(q);

    this->T1 = params->g.pow_mod(t1, p).mul_mod(params->h.pow_mod(tau1, p), p).pow_mod(-1, p);
    this->T2 = params->g.pow_mod(t2, p).mul_mod(params->h.pow_mod(tau2, p), p).pow_mod(-1, p);

    // PAPER LINES 54-56
    hasher << z;
    hasher << this->T1;
    hasher << this->T2;

    CBigNum x = CBigNum(hasher.GetHash());

    if (x == 0)
        goto try_again;

    // PAPER LINES 58-59
    CBN_vector l = VectorAdd(l0, VectorScalar(l1, x, q), q);
    CBN_vector r = VectorAdd(r0, VectorScalar(r1, x, q), q);

    // PAPER LINE 60
    this->t = InnerProduct(l, r, q);

    // TEST
    CBigNum test_t;
    CBigNum t0 = InnerProduct(l0, r0, q);
    test_t = ((t0 + (t1*x)) + (t2*x*x)) % q;
    if (test_t!=this->t)
        throw std::runtime_error("BulletproofRangeproof::Prove(): L60 Invalid test");
    else std::cout << "test passed " << test_t.ToString(16).substr(0,8) << " == " << this->t.ToString(16).substr(0,8) << std::endl;

    // PAPER LINES 61-62
    this->taux = tau1 * x;
    this->taux = this->taux + (tau2 * x.pow(2));

    for (unsigned int j = 1; j <= M; j++) // note this starts from 1
        this->taux = this->taux + (zpow[j+1]*(gamma[j-1]));

    this->mu = (x * rho) + alpha;

    // PAPER LINE 63
    hasher << x;
    hasher << this->taux;
    hasher << this->mu;
    hasher << this->t;

    CBigNum x_ip = CBigNum(hasher.GetHash());

    if (x_ip == 0)
        goto try_again;

    // These are used in the inner product rounds
    unsigned int nprime = MN;

    CBN_vector gprime(nprime);
    CBN_vector hprime(nprime);
    CBN_vector ggprime(nprime);
    CBN_vector hhprime(nprime);
    CBN_vector aprime(nprime);
    CBN_vector bprime(nprime);

    CBigNum yinv = y.inverse(q);
    CBN_vector yinvpow(nprime);

    yinvpow[0] = 1;
    yinvpow[1] = yinv;

    for (unsigned int i = 0; i < nprime; i++)
    {
        ggprime[i] = params->gis[i+1];
        hhprime[i] = params->gis[i+nprime+1];
        gprime[i] = params->gis[i+1];
        hprime[i] = params->gis[i+nprime+1].pow_mod(y.inverse(q).pow_mod(i, q), p);

        if(i > 1)
            yinvpow[i] = yinvpow[i-1].mul_mod(yinv, q);

        aprime[i] = l[i];
        bprime[i] = r[i];
    }

    this->L.resize(logMN);
    this->R.resize(logMN);

    unsigned int round = 0;

    CBN_vector w(logMN);

    CBN_vector* scale = &yinvpow;

    while (nprime > 1)
    {
        // PAPER LINE 20
        nprime /= 2;

        // PAPER LINES 21-22
        CBigNum cL = InnerProduct(VectorSlice(aprime, 0, nprime),
                                  VectorSlice(bprime, nprime, bprime.size()),
                                  q);

        CBigNum cR = InnerProduct(VectorSlice(aprime, nprime, aprime.size()),
                                  VectorSlice(bprime, 0, nprime),
                                  q);

        // PAPER LINES 23-24
//        this->L[round] = VectorExponent2Mod(VectorSlice(gprime, nprime, gprime.size()),
//                                            VectorSlice(aprime, 0, nprime),
//                                            VectorSlice(hprime, 0, nprime),
//                                            VectorSlice(bprime, nprime, bprime.size()),
//                                            p).
//                        mul_mod(p->u_inner_prod.pow_mod(cL.mul_mod(x_ip, q), p), p);

//        this->R[round] = VectorExponent2Mod(VectorSlice(gprime, 0, nprime),
//                                            VectorSlice(aprime, nprime, aprime.size()),
//                                            VectorSlice(hprime, nprime, hprime.size()),
//                                            VectorSlice(bprime, 0, nprime),
//                                            p).
//                        mul_mod(p->u_inner_prod.pow_mod(cR.mul_mod(x_ip, q), p), p);

        CBigNum tempBN = cL.mul_mod(x_ip, q);
        this->L[round] = CrossVectorExponent(nprime, ggprime, nprime, hhprime, 0, aprime, 0, bprime, nprime, scale, &params->u_inner_prod, &tempBN, p);
        tempBN = cR.mul_mod(x_ip, q);
        this->R[round] = CrossVectorExponent(nprime, ggprime, 0, hhprime, nprime, aprime, nprime, bprime, 0, scale, &params->u_inner_prod, &tempBN, p);

//        std::cout << this->L[round].ToString(16).substr(0,8) << " " << tempLL.ToString(16).substr(0,8) << std::endl;

//        if (!)
//            throw std::runtime_error("BulletproofRangeproof::Prove(): L23-24 Invalid cross vector exponent L");

//        tempBN = cR.mul_mod(x_ip, q);

//        if (!CrossVectorExponent(this->R[round], nprime, gprime, 0, hprime, nprime, aprime, nprime, bprime, 0, scale, &params->h, &tempBN, p))
//            throw std::runtime_error("BulletproofRangeproof::Prove(): L23-24 Invalid cross vector exponent R");

        // PAPER LINES 25-27
        hasher << this->L[round];
        hasher << this->R[round];

        w[round] = CBigNum(hasher.GetHash());

        if (w[round] == 0)
            goto try_again;

        CBigNum winv = w[round].inverse(q);

        // PAPER LINES 29-31
//        gprime = Hadamard(VectorExponent(VectorSlice(gprime, 0, nprime), winv, p),
//                          VectorExponent(VectorSlice(gprime, nprime, gprime.size()), w[round], p),
//                          p);

//        hprime = Hadamard(VectorExponent(VectorSlice(hprime, 0, nprime), w[round], p),
//                          VectorExponent(VectorSlice(hprime, nprime, hprime.size()), winv, p),
//                          p);

        if (nprime > 1)
        {
            ggprime = HadamardFold(ggprime, NULL, winv, w[round], p, q);
            hhprime = HadamardFold(hhprime, scale, w[round], winv, p, q);
        }

//        std::cout << "G " << gprime[0].ToString(16).substr(0,8)
//                  << "  " << ggprime[0].ToString(16).substr(0,8) << std::endl;
//        std::cout << "H " << hprime[0].ToString(16).substr(0,8)
//                  << "  " << hhprime[0].ToString(16).substr(0,8) << std::endl;


        // PAPER LINES 33-34
        aprime = VectorAdd(VectorScalar(VectorSlice(aprime, 0, nprime), w[round], q),
                           VectorScalar(VectorSlice(aprime, nprime, aprime.size()), winv, q), q);

        bprime = VectorAdd(VectorScalar(VectorSlice(bprime, 0, nprime), winv, q),
                           VectorScalar(VectorSlice(bprime, nprime, bprime.size()), w[round], q), q);

        scale = NULL;

        round += 1;
    }

    this->a = aprime[0];
    this->b = bprime[0];

    std::cout << ToString() << std::endl;
}

struct proof_data_t
{
    CBigNum x, y, z, x_ip;
    CBN_vector w;
    size_t logM, inv_offset;

    std::string ToString() const {
        std::string ret = "x: " + x.ToString(16).substr(0,8) + "\n";
        ret += "y: " + y.ToString(16).substr(0,8) + "\n";
        ret += "z: " + z.ToString(16).substr(0,8) + "\n";
        ret += "x_ip: " + x_ip.ToString(16).substr(0,8) + "\n";
        ret += "w: " + libzerocoin::toStringVector(w) + "\n";
        ret += "logM: " + to_string(logM) + "\n";
        ret += "inv_offset: " + to_string(inv_offset) + "\n";
        return ret;
    }
};

bool VerifyBulletproof(const libzerocoin::IntegerGroupParams* params, const std::vector<BulletproofRangeproof>& proofs)
{
    if (proofs.size() == 0)
        throw std::runtime_error("VerifyBulletproof(): Empty proofs vector");

    unsigned int logN = 6;
    unsigned int N = 1 << logN;

    CBigNum p = params->modulus;
    CBigNum q = params->groupOrder;

    size_t max_length = 0;
    size_t nV = 0;

    std::vector<proof_data_t> proof_data;
    proof_data.reserve(proofs.size());

    size_t inv_offset = 0;
    CBN_vector to_invert;
    to_invert.reserve(11 * sizeof(proofs));

    for (const BulletproofRangeproof proof: proofs)
    {
        if (!(proof.V.size() >= 1 && proof.L.size() == proof.R.size() &&
              proof.L.size() > 0))
            return false;

        max_length = std::max(max_length, proof.L.size());
        nV += proof.V.size();
        proof_data.resize(proof_data.size() + 1);
        proof_data_t &pd = proof_data.back();

        CHashWriter hasher(0,0);

        hasher << proof.V[0];

        for (unsigned int j = 1; j < proof.V.size(); j++)
            hasher << proof.V[j];

        hasher << proof.A;
        hasher << proof.S;

        pd.y = CBigNum(hasher.GetHash());

        hasher << pd.y;

        pd.z = CBigNum(hasher.GetHash());

        hasher << pd.z;
        hasher << proof.T1;
        hasher << proof.T2;

        pd.x = CBigNum(hasher.GetHash());

        hasher << pd.x;
        hasher << proof.taux;
        hasher << proof.mu;
        hasher << proof.t;

        pd.x_ip = CBigNum(hasher.GetHash());

        size_t M;
        for (pd.logM = 0; (M = 1<<pd.logM) <= BulletproofRangeproof::maxM && M < proof.V.size(); ++pd.logM);

        const size_t rounds = pd.logM+logN;

        pd.w.resize(rounds);
        for (size_t i = 0; i < rounds; ++i)
        {
            hasher << proof.L[i];
            hasher << proof.R[i];

            pd.w[i] = CBigNum(hasher.GetHash());
        }

        pd.inv_offset = inv_offset;
        for (size_t i = 0; i < rounds; ++i)
            to_invert.push_back(pd.w[i]);
        to_invert.push_back(pd.y);
        inv_offset += rounds + 1;
    }

    size_t maxMN = 1u << max_length;

    CBN_vector inverses(to_invert.size());

    for (size_t n = 0; n < to_invert.size(); n++) {
        inverses[n] = to_invert[n].inverse(q);
    }

    CBigNum z1 = 0;
    CBigNum z3 = 0;
    CBN_vector z4(maxMN, 0);
    CBN_vector z5(maxMN, 0);

    CBigNum y0 = 0; // tau_x
    CBigNum y1 = 0; // t-(k+z+Sum(y^i))

    CBigNum tmp;

    int proof_data_index = 0;

    std::vector<MultiexpData> multiexpdata;

    multiexpdata.reserve(nV + (2 * (10/*logM*/ + logN) + 4) * proofs.size() + 2 * maxMN);
    multiexpdata.resize(2 * maxMN);

    for (const BulletproofRangeproof proof: proofs)
    {
        const proof_data_t &pd = proof_data[proof_data_index++];

        if (proof.L.size() != logN+pd.logM)
            return false;

        const size_t M = 1 << pd.logM;
        const size_t MN = M*N;

        CBigNum weight_y = CBigNum::randBignum(q);
        CBigNum weight_z = CBigNum::randBignum(q);

        y0 = y0 - (proof.taux.mul_mod(weight_y, q));

        CBN_vector zpow = VectorPowers(pd.z, M+3);

        CBigNum k;
        CBigNum ip1y = VectorPowerSum(pd.y, MN);

        k = 0 - (zpow[2]*ip1y);

        for (size_t j = 1; j <= M; ++j)
        {
            k = k - (zpow[j+2]*ip12);
        }

        tmp = k + (pd.z*ip1y);

        tmp = (proof.t - tmp);

        y1 = y1 + (tmp * weight_y);

        for (size_t j = 0; j < proof.V.size(); j++)
        {
            tmp = zpow[j+2] * weight_y;
            multiexpdata.push_back({proof.V[j], tmp % q});
        }

        tmp = pd.x * weight_y;

        multiexpdata.push_back({proof.T1, tmp % q});

        tmp = pd.x * pd.x * weight_y;

        multiexpdata.push_back({proof.T2, tmp % q});

        multiexpdata.push_back({proof.A, weight_z % q});

        tmp = pd.x * weight_z;

        multiexpdata.push_back({proof.S, tmp % q});

        const size_t rounds = pd.logM+logN;

        CBigNum yinvpow = 1;
        CBigNum ypow = 1;

        const CBigNum *winv = &inverses[pd.inv_offset];
        const CBigNum yinv = inverses[pd.inv_offset + rounds];

        CBN_vector w_cache(1<<rounds, 1);
        w_cache[0] = winv[0];
        w_cache[1] = pd.w[0];

        for (size_t j = 1; j < rounds; ++j)
        {
            const size_t sl = 1<<(j+1);
            for (size_t s = sl; s-- > 0; --s)
            {
                w_cache[s] = w_cache[s/2] * pd.w[j];
                w_cache[s-1] = w_cache[s/2] * winv[j];
            }
        }

        for (size_t i = 0; i < MN; ++i)
        {
            CBigNum g_scalar = proof.a;
            CBigNum h_scalar;

            if (i == 0)
                h_scalar = proof.b;
            else
                h_scalar = proof.b * yinvpow;

            g_scalar = g_scalar * w_cache[i];
            h_scalar = h_scalar * w_cache[(~i) & (MN-1)];

            g_scalar = g_scalar + pd.z;

            tmp = zpow[2+i/N] * twoN[i%N];

            if (i == 0)
            {
                tmp = tmp + pd.z;
                h_scalar = h_scalar - tmp;
            }
            else
            {
                tmp = tmp + (pd.z*ypow);
                h_scalar = h_scalar - (tmp * yinvpow);
            }

            z4[i] = z4[i] - (g_scalar * weight_z);
            z5[i] = z5[i] - (h_scalar * weight_z);

            if (i == 0)
            {
                yinvpow = yinv;
                ypow = pd.y;
            }
            else if (i != MN-1)
            {
                yinvpow = yinvpow * yinv;
                ypow = ypow * pd.y;
            }
        }

        z1 = z1 + (proof.mu * weight_z);

        for (size_t i = 0; i < rounds; ++i)
        {
            tmp = pd.w[i] * pd.w[i];
            tmp = tmp * weight_z;

            multiexpdata.push_back({proof.L[i], tmp % q});

            tmp = winv[i] * winv[i];
            tmp = tmp * weight_z;

            multiexpdata.push_back({proof.R[i], tmp % q});
        }

        tmp = proof.t - (proof.a*proof.b);
        tmp = tmp * pd.x_ip;
        z3 = z3 + (tmp * weight_z);
    }

    tmp = y0 - z1;

    multiexpdata.push_back({params->g, tmp % q});

    tmp = z3 - y1;

    multiexpdata.push_back({params->h, tmp % q});

    for (size_t i = 0; i < maxMN; ++i)
    {
        multiexpdata[i * 2] = {params->gis[i+1], z4[i] % q};
        multiexpdata[i * 2 + 1] = {params->gis[maxMN+i+1], z5[i] % q};
    }

    CBigNum result = MultiExp(multiexpdata, p);

    std::cout << "result is " << result.ToString(16) << std::endl;

    return result == 1;
}

bool BulletproofRangeproof::Verify()
{
    unsigned int maxMN = pow(2, L.size());

    const size_t logN = 6;
    const size_t N = 1<<logN;

    CBigNum p = params->modulus;
    CBigNum q = params->groupOrder;

    CBigNum y0 = 0; // tau_x
    CBigNum y1 = 0; // t-(k+z+Sum(y^i))
    CBigNum Y2 = 1; // z-V sum
    CBigNum Y3 = 1; // xT_1
    CBigNum Y4 = 1; // x^2T_2

    CBigNum Z0 = 1; // A + xS
    CBigNum z1 = 0; // mu
    CBigNum Z2 = 1; // Li/Ri sum
    CBigNum z3 = 0; // (t-ab)x_ip
    CBN_vector z4(maxMN); // gscalar sum
    CBN_vector z5(maxMN); // hscalar sum

    for (unsigned int i = 0; i < maxMN; i++)
    {
        z4[i] = 0;
        z5[i] = 0;
    }

    unsigned int logMN = L.size();
    unsigned int M = pow(2,logMN)/N;

    const size_t MN = M*N;

    CBigNum weight = CBigNum::randBignum(q);

    CHashWriter hasher(0,0);

    for (unsigned int j = 0; j < M; j++)
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

    CBN_vector yMN = VectorPowers(y, MN);
    CBigNum ip1yMN = InnerProduct(VectorPowers(1, MN), yMN, q);
    CBN_vector zpow = VectorPowers(z, M+3);

    CBigNum k = 0 - zpow[2]*ip1yMN;

    for (unsigned int j = 1; j <= M; j++) // note this starts from 1
        k = k - zpow[j+2]*ip12;

    y1 = y1 + (t - (k + (z * ip1yMN)) * weight);

    CBigNum bnTemp = 1;

    for (unsigned int j = 0; j < M; j++)
    {
        bnTemp = bnTemp.mul_mod(this->V[j].pow_mod(zpow[j+2], p), p);
    }

    Y2 = Y2.mul_mod(bnTemp.pow_mod(weight, p), p);
    Y3 = Y3.mul_mod(T1.pow_mod(x * weight, p), p);
    Y4 = Y4.mul_mod(T2.pow_mod(x * x * weight, p), p);

    // PAPER LINE 66

    Z0 = Z0.mul_mod((A.mul_mod(S.pow_mod(x, p), p)).pow_mod(weight, p), p);

    // PAPER LINES 25-27
    // The inner product challenges are computed per round
    CBN_vector w = CBN_vector(logMN);

    hasher << L[0];
    hasher << R[0];

    w[0] = CBigNum(hasher.GetHash());

    if (logMN > 1)
    {
        for (unsigned int j = 1; j < logMN; j++)
        {
            hasher << L[j];
            hasher << R[j];

            w[j] = CBigNum(hasher.GetHash());
        }
    }

    // Basically PAPER LINES 28-30
    // Compute the curvepoints from G[i] and H[i]

    for (unsigned int j = 0; j < MN; j++)
    {
        // Convert the index to binary IN REVERSE and construct the scalar exponent
        unsigned int index = j;
        CBigNum gScalar = a;
        CBigNum hScalar = b * (y.inverse(q).pow(j));

        for (unsigned int jj = logMN-1; jj >= 0; jj--)
        {
            unsigned int J = w.size() - jj - 1; // because this is done in reverse bit order
            unsigned int basePow = pow(2, jj); // assumes we don't get too big
            if (index / basePow == 0) // bit is zero
            {
                gScalar = gScalar.mul_mod(w[J].inverse(q), q);
                hScalar = hScalar.mul_mod(w[J], q);
            }
            else // bit is one
            {
                gScalar = gScalar.mul_mod(w[J], q);
                hScalar = hScalar.mul_mod(w[J].inverse(q), q);
                index -= basePow;
            }
        }

        gScalar = gScalar + z;
        hScalar = (hScalar - z.mul_mod(y.pow(j), q) + zpow[2+j/N].mul_mod(CBigNum(2).pow(j%N), q).mul_mod(y.inverse(q).pow(j), q)) % q;

        // Now compute the basepoint's scalar multiplication
        z4[j] = z4[j] + (gScalar * (weight));
        z5[j] = z5[j] + (hScalar * (weight));
    }

    // PAPER LINE 31
    z1 = z1 + (mu * (weight));

    bnTemp = 1;
    for (unsigned int j = 0; j < logMN; j++)
    {
        bnTemp = bnTemp.mul_mod(L[j].pow_mod(w[j].pow(2), p), p);
        bnTemp = bnTemp.mul_mod(R[j].pow_mod(w[j].inverse(q).pow(2), p), p);
    }
    Z2 = Z2.mul_mod(bnTemp.pow_mod(weight, p), p);
    z3 = z3 + ((t - (a * b)) * (x_ip) * (weight));

    // Perform the first- and second-stage check on all proofs at once
    CBigNum Check1 = 1;
    Check1 = Check1.mul_mod(params->g.pow_mod(y0, p), p);
    Check1 = Check1.mul_mod(params->h.pow_mod(y1, p), p);
    Check1 = Check1.mul_mod(Y2.pow_mod(-1, p), p);
    Check1 = Check1.mul_mod(Y3.pow_mod(-1, p), p);
    Check1 = Check1.mul_mod(Y4.pow_mod(-1, p), p);

    if (Check1 != 0)
    {
        std::cout << "Failed first-stage check " << Check1.ToString(16).substr(0,8) << std::endl;
        return false;
    }

    CBigNum Check2 = Z0;
    Check2 = Check2.mul_mod(params->g.pow_mod(0-z1, p), p);
    Check2 = Check2.mul_mod(Z2, p);
    Check2 = Check2.mul_mod(params->h.pow_mod(z3, p), p);

    for (unsigned int i = 0; i < maxMN; i++)
    {
        Check2 = Check2.mul_mod(params->gis[i+1].pow_mod(0 - z4[i], p), p);
        Check2 = Check2.mul_mod(params->gis[i+maxMN+1].pow_mod(0 - z5[i], p), p);
    }

    if (Check2 != 0)
    {
        std::cout << "Failed second-stage check" << Check2.ToString(16).substr(0,8) << std::endl;
        return false;
    }

    return true;

}


