// Copyright (c) 2012-2018 The Bitcoin Core developers
// Copyright (c) 2018-2019 Chaintope Inc.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>

#include <key_io.h>
#include <script/script.h>
#include <uint256.h>
#include <util.h>
#include <utilstrencodings.h>
#include <test/test_keys_helper.h>
#include <test/test_tapyrus.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

static const std::string strSecret1 = "5HxWvvfubhXpYYpS3tJkw6fq9jE9j18THftkZjHHfmFiWtmAbrj";
static const std::string strSecret2 = "5KC4ejrDjv152FGwP386VD1i2NYc5KkfSMyv1nGy1VGDxGHqVY3";
static const std::string strSecret1C = "Kwr371tjA9u2rFSMZjTNun2PXXP3WPZu2afRHTcta6KxEUdm1vEw";
static const std::string strSecret2C = "L3Hq7a8FEQwJkW1M2GNKDW28546Vp5miewcCzSqUD9kCAXrJdS3g";
static const std::string addr1 = "1QFqqMUD55ZV3PJEJZtaKCsQmjLT6JkjvJ";
static const std::string addr2 = "1F5y5E5FMc5YzdJtB9hLaUe43GDxEKXENJ";
static const std::string addr1C = "1NoJrossxPBKfCHuJXT4HadJrXRE9Fxiqs";
static const std::string addr2C = "1CRj2HyM1CXWzHAXLQtiGLyggNT9WQqsDs";
static const std::string caddr1 = "vhZAy3sWHvBvvAtXPqqrw4Y9pBWgcFJ1emPqowMcGNweoLQ4CHDXD6q5JJSir7XDa3ETsuJNQZjvd5"; //cp2kh address
static const std::string caddr2 = "w6B4Zz97JtEUFV3dRTxKRjVnCYy6nTbZ4mojZ4W1g6DLFvUx3C5DNr9vF1uuFF6xYBqAzTQdYkJcg5"; //invalid cp2kh address

static const std::string strAddressBad = "1HV9Lc3sNHZxwj4Zk6fB38tEmBryq2cBiF";


BOOST_FIXTURE_TEST_SUITE(key_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(key_test1)
{
    CKey key1  = DecodeSecret(strSecret1);
    BOOST_CHECK(key1.IsValid() && !key1.IsCompressed());
    CKey key2  = DecodeSecret(strSecret2);
    BOOST_CHECK(key2.IsValid() && !key2.IsCompressed());
    CKey key1C = DecodeSecret(strSecret1C);
    BOOST_CHECK(key1C.IsValid() && key1C.IsCompressed());
    CKey key2C = DecodeSecret(strSecret2C);
    BOOST_CHECK(key2C.IsValid() && key2C.IsCompressed());
    CKey bad_key = DecodeSecret(strAddressBad);
    BOOST_CHECK(!bad_key.IsValid());

    CPubKey pubkey1  = key1. GetPubKey();
    CPubKey pubkey2  = key2. GetPubKey();
    CPubKey pubkey1C = key1C.GetPubKey();
    CPubKey pubkey2C = key2C.GetPubKey();

    BOOST_CHECK(key1.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key1C.VerifyPubKey(pubkey1));
    BOOST_CHECK(key1C.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key1C.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key1C.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key2.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key2.VerifyPubKey(pubkey1C));
    BOOST_CHECK(key2.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key2.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key2C.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key2C.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key2C.VerifyPubKey(pubkey2));
    BOOST_CHECK(key2C.VerifyPubKey(pubkey2C));

    ColorIdentifier colorId;
    BOOST_CHECK(DecodeDestination(addr1, colorId) == CTxDestination(pubkey1.GetID()));
    BOOST_CHECK(colorId.type == TokenTypes::NONE);
    BOOST_CHECK(colorId.toVector().size() == 1);
    BOOST_CHECK(DecodeDestination(addr2, colorId)  == CTxDestination(pubkey2.GetID()));
    BOOST_CHECK(colorId.type == TokenTypes::NONE);
    BOOST_CHECK(colorId.toVector().size() == 1);
    BOOST_CHECK(DecodeDestination(addr1C, colorId) == CTxDestination(pubkey1C.GetID()));
    BOOST_CHECK(colorId.type == TokenTypes::NONE);
    BOOST_CHECK(colorId.toVector().size() == 1);
    BOOST_CHECK(DecodeDestination(addr2C, colorId) == CTxDestination(pubkey2C.GetID()));
    BOOST_CHECK(colorId.type == TokenTypes::NONE);
    BOOST_CHECK(colorId.toVector().size() == 1);
    //valid color address
    BOOST_CHECK(DecodeDestination(caddr1, colorId) == CTxDestination(pubkey1.GetID()));
    BOOST_CHECK(colorId.type == TokenTypes::REISSUABLE);
    BOOST_CHECK(colorId.toVector().size() == 33);
    //invalid color address
    BOOST_CHECK(DecodeDestination(caddr2, colorId) == CTxDestination(pubkey1.GetID()));
    BOOST_CHECK(colorId.type == TokenTypes::NONE);
    BOOST_CHECK(colorId.toVector().size() == 1);

    for (int n=0; n<16; n++)
    {
        std::string strMsg = strprintf("Very secret message %i: 11", n);
        uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());

        // normal ecdsa signatures

        std::vector<unsigned char> sign1, sign2, sign1C, sign2C, signSc1, signSc2, signSc1C, signSc2C;

        BOOST_CHECK(key1.Sign_ECDSA(hashMsg, sign1));
        BOOST_CHECK(key2.Sign_ECDSA(hashMsg, sign2));
        BOOST_CHECK(key1C.Sign_ECDSA(hashMsg, sign1C));
        BOOST_CHECK(key2C.Sign_ECDSA(hashMsg, sign2C));

        BOOST_CHECK( pubkey1.Verify_ECDSA(hashMsg, sign1));
        BOOST_CHECK(!pubkey1.Verify_ECDSA(hashMsg, sign2));
        BOOST_CHECK( pubkey1.Verify_ECDSA(hashMsg, sign1C));
        BOOST_CHECK(!pubkey1.Verify_ECDSA(hashMsg, sign2C));

        BOOST_CHECK(!pubkey2.Verify_ECDSA(hashMsg, sign1));
        BOOST_CHECK( pubkey2.Verify_ECDSA(hashMsg, sign2));
        BOOST_CHECK(!pubkey2.Verify_ECDSA(hashMsg, sign1C));
        BOOST_CHECK( pubkey2.Verify_ECDSA(hashMsg, sign2C));

        BOOST_CHECK( pubkey1C.Verify_ECDSA(hashMsg, sign1));
        BOOST_CHECK(!pubkey1C.Verify_ECDSA(hashMsg, sign2));
        BOOST_CHECK( pubkey1C.Verify_ECDSA(hashMsg, sign1C));
        BOOST_CHECK(!pubkey1C.Verify_ECDSA(hashMsg, sign2C));

        BOOST_CHECK(!pubkey2C.Verify_ECDSA(hashMsg, sign1));
        BOOST_CHECK( pubkey2C.Verify_ECDSA(hashMsg, sign2));
        BOOST_CHECK(!pubkey2C.Verify_ECDSA(hashMsg, sign1C));
        BOOST_CHECK( pubkey2C.Verify_ECDSA(hashMsg, sign2C));

        // compact ecdsa signatures (with key recovery)

        std::vector<unsigned char> csign1, csign2, csign1C, csign2C;

        BOOST_CHECK(key1.SignCompact (hashMsg, csign1));
        BOOST_CHECK(key2.SignCompact (hashMsg, csign2));
        BOOST_CHECK(key1C.SignCompact(hashMsg, csign1C));
        BOOST_CHECK(key2C.SignCompact(hashMsg, csign2C));

        CPubKey rkey1, rkey2, rkey1C, rkey2C;

        BOOST_CHECK(rkey1.RecoverCompact (hashMsg, csign1));
        BOOST_CHECK(rkey2.RecoverCompact (hashMsg, csign2));
        BOOST_CHECK(rkey1C.RecoverCompact(hashMsg, csign1C));
        BOOST_CHECK(rkey2C.RecoverCompact(hashMsg, csign2C));

        BOOST_CHECK(rkey1  == pubkey1);
        BOOST_CHECK(rkey2  == pubkey2);
        BOOST_CHECK(rkey1C == pubkey1C);
        BOOST_CHECK(rkey2C == pubkey2C);

        // schnorr signatures
        BOOST_CHECK(key1.Sign_Schnorr(hashMsg, signSc1));
        BOOST_CHECK(key2.Sign_Schnorr(hashMsg, signSc2));
        BOOST_CHECK(key1C.Sign_Schnorr(hashMsg, signSc1C));
        BOOST_CHECK(key2C.Sign_Schnorr(hashMsg, signSc2C));

        BOOST_CHECK(signSc1.size() == 64);
        BOOST_CHECK(signSc2.size() == 64);
        BOOST_CHECK(signSc1C.size() == 64);
        BOOST_CHECK(signSc2C.size() == 64);

        BOOST_CHECK( pubkey1.Verify_Schnorr(hashMsg, signSc1));
        BOOST_CHECK(!pubkey1.Verify_Schnorr(hashMsg, signSc2));
        BOOST_CHECK( pubkey1.Verify_Schnorr(hashMsg, signSc1C));
        BOOST_CHECK(!pubkey1.Verify_Schnorr(hashMsg, signSc2C));

        BOOST_CHECK(!pubkey2.Verify_Schnorr(hashMsg, signSc1));
        BOOST_CHECK( pubkey2.Verify_Schnorr(hashMsg, signSc2));
        BOOST_CHECK(!pubkey2.Verify_Schnorr(hashMsg, signSc1C));
        BOOST_CHECK( pubkey2.Verify_Schnorr(hashMsg, signSc2C));

        BOOST_CHECK( pubkey1C.Verify_Schnorr(hashMsg, signSc1));
        BOOST_CHECK(!pubkey1C.Verify_Schnorr(hashMsg, signSc2));
        BOOST_CHECK( pubkey1C.Verify_Schnorr(hashMsg, signSc1C));
        BOOST_CHECK(!pubkey1C.Verify_Schnorr(hashMsg, signSc2C));

        BOOST_CHECK(!pubkey2C.Verify_Schnorr(hashMsg, signSc1));
        BOOST_CHECK( pubkey2C.Verify_Schnorr(hashMsg, signSc2));
        BOOST_CHECK(!pubkey2C.Verify_Schnorr(hashMsg, signSc1C));
        BOOST_CHECK( pubkey2C.Verify_Schnorr(hashMsg, signSc2C));
    }

    // test deterministic signing

    std::vector<unsigned char> detsig, detsigc;
    std::string strMsg = "Very deterministic message";
    uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());
    BOOST_CHECK(key1.Sign_ECDSA(hashMsg, detsig));
    BOOST_CHECK(key1C.Sign_ECDSA(hashMsg, detsigc));
    BOOST_CHECK(detsig == detsigc);
    BOOST_CHECK(detsig == ParseHex("304402205dbbddda71772d95ce91cd2d14b592cfbc1dd0aabd6a394b6c2d377bbe59d31d022014ddda21494a4e221f0824f0b8b924c43fa43c0ad57dccdaa11f81a6bd4582f6"));
    BOOST_CHECK(key2.Sign_ECDSA(hashMsg, detsig));
    BOOST_CHECK(key2C.Sign_ECDSA(hashMsg, detsigc));
    BOOST_CHECK(detsig == detsigc);
    BOOST_CHECK(detsig == ParseHex("3044022052d8a32079c11e79db95af63bb9600c5b04f21a9ca33dc129c2bfa8ac9dc1cd5022061d8ae5e0f6c1a16bde3719c64c2fd70e404b6428ab9a69566962e8771b5944d"));
    BOOST_CHECK(key1.SignCompact(hashMsg, detsig));
    BOOST_CHECK(key1C.SignCompact(hashMsg, detsigc));
    BOOST_CHECK(detsig == ParseHex("1c5dbbddda71772d95ce91cd2d14b592cfbc1dd0aabd6a394b6c2d377bbe59d31d14ddda21494a4e221f0824f0b8b924c43fa43c0ad57dccdaa11f81a6bd4582f6"));
    BOOST_CHECK(detsigc == ParseHex("205dbbddda71772d95ce91cd2d14b592cfbc1dd0aabd6a394b6c2d377bbe59d31d14ddda21494a4e221f0824f0b8b924c43fa43c0ad57dccdaa11f81a6bd4582f6"));
    BOOST_CHECK(key2.SignCompact(hashMsg, detsig));
    BOOST_CHECK(key2C.SignCompact(hashMsg, detsigc));
    BOOST_CHECK(detsig == ParseHex("1c52d8a32079c11e79db95af63bb9600c5b04f21a9ca33dc129c2bfa8ac9dc1cd561d8ae5e0f6c1a16bde3719c64c2fd70e404b6428ab9a69566962e8771b5944d"));
    BOOST_CHECK(detsigc == ParseHex("2052d8a32079c11e79db95af63bb9600c5b04f21a9ca33dc129c2bfa8ac9dc1cd561d8ae5e0f6c1a16bde3719c64c2fd70e404b6428ab9a69566962e8771b5944d"));
    BOOST_CHECK(key1.Sign_Schnorr(hashMsg, detsig));
    BOOST_CHECK(key1C.Sign_Schnorr(hashMsg, detsigc));
    BOOST_CHECK(detsig == detsigc);
    BOOST_CHECK(detsig == ParseHex("0567cbade8656cff3bb08d00913d59363273c32ea66130cf0c9b1be8e874b8bcb0e62372c22e8ecd34ffeadda493beb221e52bf23413cc6c3abdcdfc03d0ed52"));
    BOOST_CHECK(key2.Sign_Schnorr(hashMsg, detsig));
    BOOST_CHECK(key2C.Sign_Schnorr(hashMsg, detsigc));
    BOOST_CHECK(detsig == detsigc);
    BOOST_CHECK(detsig == ParseHex("064623e23b59e1bd304156fb20c197eee23e6d10e021664aef3878364d9d5e175916f7909c9358192e9c1510ebb466b085e726aab0d71c6ef9f298b53ea179aa"));
}

BOOST_AUTO_TEST_CASE(key_signature_tests)
{
    // When entropy is specified, we should see at least one high R signature within 20 signatures
    CKey key = DecodeSecret(strSecret1);
    std::string msg = "A message to be signed";
    uint256 msg_hash = Hash(msg.begin(), msg.end());
    std::vector<unsigned char> sig;
    bool found = false;

    for (int i = 1; i <=20; ++i) {
        sig.clear();
        key.Sign_ECDSA(msg_hash, sig, false, i);
        found = sig[3] == 0x21 && sig[4] == 0x00;
        if (found) {
            break;
        }
    }
    BOOST_CHECK(found);

    // When entropy is not specified, we should always see low R signatures that are less than 70 bytes in 256 tries
    // We should see at least one signature that is less than 70 bytes.
    found = true;
    bool found_small = false;
    for (int i = 0; i < 256; ++i) {
        sig.clear();
        std::string msg = "A message to be signed" + std::to_string(i);
        msg_hash = Hash(msg.begin(), msg.end());
        key.Sign_ECDSA(msg_hash, sig);
        found = sig[3] == 0x20;
        BOOST_CHECK(sig.size() <= 70);
        found_small |= sig.size() < 70;
    }
    BOOST_CHECK(found);
    BOOST_CHECK(found_small);
}

BOOST_AUTO_TEST_CASE(pubkey_combine_tests)
{
    auto pubkeys = validPubKeys(15);
    CPubKey result = PubKeyCombine(pubkeys);
    size_t publen = CPubKey::COMPRESSED_PUBLIC_KEY_SIZE;

    std::string pubkeyString = HexStr(result.data(), result.data() + publen);
    BOOST_CHECK_EQUAL(pubkeyString, "02d7bbe714a08f73b17a3e5dcbca523470e9de5ee6c92f396beb954b8a2cdf4388");
}

BOOST_AUTO_TEST_SUITE_END()
