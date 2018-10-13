// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_KEYSTORE_H
#define NAVCOIN_KEYSTORE_H

#include "key.h"
#include "pubkey.h"
#include "script/script.h"
#include "script/standard.h"
#include "sync.h"

#include <boost/signals2/signal.hpp>
#include <boost/variant.hpp>

/** A virtual base class for key stores */
class CKeyStore
{
protected:
    mutable CCriticalSection cs_KeyStore;

public:
    virtual ~CKeyStore() {}

    //! Add a key to the store.
    virtual bool AddKeyPubKey(const CKey &key, const CPubKey &pubkey) =0;
    virtual bool AddKey(const CKey &key);

    //! Check whether a key corresponding to a given address is present in the store.
    virtual bool HaveKey(const CKeyID &address) const =0;
    virtual bool GetKey(const CKeyID &address, CKey& keyOut) const =0;
    virtual void GetKeys(std::set<CKeyID> &setAddress) const =0;
    virtual bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const =0;

    //! Support for BIP 0013 : see https://github.com/navcoin/bips/blob/master/bip-0013.mediawiki
    virtual bool AddCScript(const CScript& redeemScript) =0;
    virtual bool HaveCScript(const CScriptID &hash) const =0;
    virtual bool GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const =0;

    //! Support for Watch-only addresses
    virtual bool AddWatchOnly(const CScript &dest) =0;
    virtual bool RemoveWatchOnly(const CScript &dest) =0;
    virtual bool HaveWatchOnly(const CScript &dest) const =0;
    virtual bool HaveWatchOnly() const =0;

    //! ZeroCoin Address Parameters
    virtual bool GetObfuscationJ(CBigNum& oj) const =0;
    virtual bool GetObfuscationK(CBigNum& ok) const =0;
    virtual bool GetBlindingCommitment(CBigNum& bc) const =0;
    virtual bool GetZeroKey(CKey& zk) const =0;
    virtual bool SetObfuscationJ(const CBigNum& oj) =0;
    virtual bool SetObfuscationK(const CBigNum& ok) =0;
    virtual bool SetBlindingCommitment(const CBigNum& bc) =0;
    virtual bool SetZeroKey(const CKey& zk) =0;

};

typedef std::map<CKeyID, CKey> KeyMap;
typedef std::map<CKeyID, CPubKey> WatchKeyMap;
typedef std::map<CScriptID, CScript > ScriptMap;
typedef std::set<CScript> WatchOnlySet;
struct ZeroCoinAddressParameters {
    CBigNum obfuscationJ;
    CBigNum obfuscationK;
    CBigNum blindingCommitment;
    CKey zerokey;
    void SetToZero() {
        obfuscationJ = CBigNum();
        obfuscationK = CBigNum();
    }
};
struct CryptedZeroCoinAddressParameters {
    std::vector<unsigned char> obfuscationJ;
    std::vector<unsigned char> obfuscationK;
    CBigNum blindingCommitment;
    CKey zerokey;
};

/** Basic key store, that keeps keys in an address->secret map */
class CBasicKeyStore : public CKeyStore
{
protected:
    KeyMap mapKeys;
    WatchKeyMap mapWatchKeys;
    ScriptMap mapScripts;
    WatchOnlySet setWatchOnly;
    ZeroCoinAddressParameters zcParameters;

public:
    bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey);
    bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const;
    bool HaveKey(const CKeyID &address) const
    {
        bool result;
        {
            LOCK(cs_KeyStore);
            result = (mapKeys.count(address) > 0);
        }
        return result;
    }
    void GetKeys(std::set<CKeyID> &setAddress) const
    {
        setAddress.clear();
        {
            LOCK(cs_KeyStore);
            KeyMap::const_iterator mi = mapKeys.begin();
            while (mi != mapKeys.end())
            {
                setAddress.insert((*mi).first);
                mi++;
            }
        }
    }
    bool GetKey(const CKeyID &address, CKey &keyOut) const
    {
        {
            LOCK(cs_KeyStore);
            KeyMap::const_iterator mi = mapKeys.find(address);
            if (mi != mapKeys.end())
            {
                keyOut = mi->second;
                return true;
            }
        }
        return false;
    }
    virtual bool AddCScript(const CScript& redeemScript);
    virtual bool HaveCScript(const CScriptID &hash) const;
    virtual bool GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const;

    virtual bool AddWatchOnly(const CScript &dest);
    virtual bool RemoveWatchOnly(const CScript &dest);
    virtual bool HaveWatchOnly(const CScript &dest) const;
    virtual bool HaveWatchOnly() const;

    bool GetObfuscationJ(CBigNum& oj) const {
        if(zcParameters.obfuscationJ == CBigNum())
            return false;
        oj = zcParameters.obfuscationJ;
        return true;
    }
    bool GetObfuscationK(CBigNum& ok) const {
        if(zcParameters.obfuscationK == CBigNum())
            return false;
        ok = zcParameters.obfuscationK;
        return true;

    }
    bool GetBlindingCommitment(CBigNum& bc) const {
        if(zcParameters.blindingCommitment == CBigNum())
            return false;
        bc = zcParameters.blindingCommitment;
        return true;

    }
    bool GetZeroKey(CKey& zk) const {
        if(!zcParameters.zerokey.IsValid())
            return false;

        zk = zcParameters.zerokey;
        return true;

    }
    bool SetObfuscationJ(const CBigNum& oj) {
        if(oj == CBigNum())
            return false;
        zcParameters.obfuscationJ = oj;
        return true;
    }
    bool SetObfuscationK(const CBigNum& ok) {
        if(ok == CBigNum())
            return false;
        zcParameters.obfuscationK = ok;
        return true;
    }
    bool SetBlindingCommitment(const CBigNum& bc) {
        if(bc == CBigNum())
            return false;
        zcParameters.blindingCommitment = bc;
        return true;
    }
    bool SetZeroKey(const CKey& zk) {
        if(!zk.IsValid())
            return false;
        zcParameters.zerokey = zk;
        return true;
    }
};

typedef std::vector<unsigned char, secure_allocator<unsigned char> > CKeyingMaterial;
typedef std::map<CKeyID, std::pair<CPubKey, std::vector<unsigned char> > > CryptedKeyMap;

#endif // NAVCOIN_KEYSTORE_H
