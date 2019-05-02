// Copyright (c) 2018 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/test_bitcoin.h>
#include <policy/policy.h>
#include <script/interpreter.h>
#include <boost/test/unit_test.hpp>
#include <uint256.h>
#include <array>
#include <script/sign.h>
#include <key.h>

#include <key_io.h>
#include <chainparams.h>
//#include <chainparams.cpp>
#include <test/test_keys_helper.h>


BOOST_FIXTURE_TEST_SUITE(combineblocksigs_tests, BasicTestingSetup);

struct testKeys{
    CKey key[3];

    testKeys(){
        //private keys from test_keys_helper.h
        //corresponding public keys are:
        //03831a69b809833ab5b32612eaf489bfea35a7321b1ca15b11d88131423fafc
        //02ce7edc292d7b747fab2f23584bbafaffde5c8ff17cf689969614441e527b90
        //02785a891f323acd6ceffc59bb14304410595914267c50467e51c87142acbb5e

        const unsigned char vchKey0[32] = {0xdb,0xb9,0xd1,0x96,0x37,0x01,0x82,0x67,0x26,0x8d,0xfc,0x2c,0xc7,0xae,0xc0,0x7e,0x72,0x17,0xc1,0xa2,0xd6,0x73,0x3e,0x11,0x84,0xa0,0x90,0x92,0x73,0xbf,0x07,0x8b};
        const unsigned char vchKey1[32] = {0xae,0x6a,0xe8,0xe5,0xcc,0xbf,0xb0,0x45,0x90,0x40,0x59,0x97,0xee,0x2d,0x52,0xd2,0xb3,0x30,0x72,0x61,0x37,0xb8,0x75,0x05,0x3c,0x36,0xd9,0x4e,0x97,0x4d,0x16,0x2f};
        const unsigned char vchKey2[32] = {0x0d,0xbb,0xe8,0xe4,0xae,0x42,0x5a,0x6d,0x26,0x87,0xf1,0xa7,0xe3,0xba,0x17,0xbc,0x98,0xc6,0x73,0x63,0x67,0x90,0xf1,0xb8,0xad,0x91,0x19,0x3c,0x05,0x87,0x5e,0xf1};

        key[0].Set(vchKey0, vchKey0 + 32, true);
        key[1].Set(vchKey1, vchKey1 + 32, true);
        key[2].Set(vchKey2, vchKey2 + 32, true);
    }
};

CProof createSignedBlockProof(CBlock &block)
{
    testKeys validKeys;

    // create blockProof with 3 signatures on the block hash
    CProof blockProof;
    std::vector<unsigned char> vchSig;
    uint256 blockHash = block.GetHashForSign();

    validKeys.key[0].Sign(blockHash, vchSig);
    blockProof.addSignature(vchSig);

    validKeys.key[1].Sign(blockHash, vchSig);
    blockProof.addSignature(vchSig);

    validKeys.key[2].Sign(blockHash, vchSig);
    blockProof.addSignature(vchSig);

    return blockProof;
}
//only declaration here to help putting the large string of serialized block data to the end of the file
std::string getSignedTestBlock();

BOOST_AUTO_TEST_CASE(combineblocksigs_test) {
    // Get a block
    CBlock block = getBlock();
    CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
    ssBlock << block;
    unsigned int blocksize= ssBlock.size();

    CProof blockProof = createSignedBlockProof(block);

    // serialize blockProof to get its size
    CDataStream ssBlockProof(SER_NETWORK, PROTOCOL_VERSION);
    ssBlockProof << blockProof;

    const MultisigCondition& signedBlocksCondition = Params().GetSignedBlocksCondition();
    // add proof to the block
    BOOST_CHECK(block.AbsorbBlockProof(blockProof, signedBlocksCondition));
    
    ssBlock.clear();
    ssBlock << block;

    BOOST_CHECK_EQUAL(ssBlock.size(), blocksize +  ssBlockProof.size() - 1);//-1 to account for no proof "00" in "blocksize"

    std::string blockHex = HexStr(ssBlock.begin(), ssBlock.end());
    BOOST_CHECK_EQUAL(blockHex, getSignedTestBlock());
}

BOOST_AUTO_TEST_CASE(invlalidblocksig_test) {
    // Get a block
    CBlock block = getBlock();
    CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
    ssBlock << block;
    unsigned int blocksize= ssBlock.size();

    CProof blockProof = createSignedBlockProof(block);

    //invalidate signature:
    // edit first <len> in [<30> <len> <02> <len R> <R> <02> <len S> <S>] 
    blockProof[2][2] = 0x30 ;

    const MultisigCondition& signedBlocksCondition = Params().GetSignedBlocksCondition();
    //returns false as all signatures in proof are not added to the block
    BOOST_CHECK_EQUAL(false, block.AbsorbBlockProof(blockProof, signedBlocksCondition));
    
    ssBlock.clear();
    ssBlock << block;

    // only valid sig are added to the block
    BOOST_CHECK_EQUAL(ssBlock.size(), blocksize + blockProof[0].size() + 1 + blockProof[1].size() + 1);
}

BOOST_AUTO_TEST_CASE(orderingblocksig_test) {
    // Get a block
    CBlock block = getBlock();
    CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
    ssBlock << block;

    CProof blockProof = createSignedBlockProof(block);

    uint256 blockHash = block.GetHashForSign();
    const MultisigCondition& signedBlocksCondition = Params().GetSignedBlocksCondition();

    //check whether signatures are ordered according to the order of public keys
    //map: public key index = signature index
    std::map<int, int> indexMap;
    for(int i = 0, size = signedBlocksCondition.pubkeys.size(); i < size; i++ ) //public keys
    {
        for (int j = 0; j < 3; j++ ) //signatures
        {
            //verify signature
            if (signedBlocksCondition.pubkeys[i].Verify(blockHash, blockProof[j]))
            {
                indexMap[i] = j;
                break;
            }
            else 
             indexMap[i] = -1;
        }
    }
    
    BOOST_CHECK(block.AbsorbBlockProof(blockProof, signedBlocksCondition));

    //compare order of proof inside the block with expected order in the map
    CProof::iterator blockIter = block.proof.begin();
    int count = 0;
    for(auto &mapItem : indexMap)
    {
        if(mapItem.second != -1)
        {
            count++;
            BOOST_CHECK_EQUAL(blockIter->size(), blockProof[mapItem.second].size());
            for(auto i = 0UL; i < blockIter->size(); i++)
                BOOST_CHECK_EQUAL(blockIter->data()[i], blockProof[mapItem.second][i]);
            
            blockIter++;
            if(blockIter == block.proof.end())
                break;
        }
    }
    BOOST_CHECK_EQUAL(count, 3);
}

std::string getSignedTestBlock()
{ 
    /* serialized proof is :
    346304422071f1e3217fab13a8f9d4aec53af7bdb93913a15e1cc01b33a2227415a5ed3e22201a26441d61fcc9a890c29599b2aa848ea46299dbb89ce627ec82abdd6ec463044220587837bc7132856dadac26608fa5b0d44ce81d772c1dd43aebcf5ce33a56582204987121ae416606328b050af5f7e84994b4130ad32a994eb50f78c1ef5fb53463044220e85b4a88774fb62b2c8c4ee1372bd7bc617e3b063d6657b3c19f84eec65220fec80d11fdc29497a35b8757a14b284641ff4c2c35ce67fa1d3fb5e22d32a
    */
    return 
    "0100000090f0a9f110702f808219ebea1173056042a714bad51b916cb6800000000000005275289558f51c9966699404ae2294730c3c9f9bda53523ce50e9b95e558da2fdb261b4d0346304402200e85b4a88774fb62b2c8c4ee1372bd7bc617e3b0063d0606057b3c19f84eec6502200fec80d11fdc29497a35b8757a14b284641ff4c2c35ce67fa1d30fb5e22d320a46304402205807837bc71328560dadac26608fa5b0d44ce81d772c1dd43aebcf5ce33a565802204987121ae4166063028b0050af5f7e84994b4130ad32a994eb50f78c1ef5fb53463044022071f1e3217fab13a8f9d4aec53af7bdb93913a15e1cc01b33a20227415a5ed3e202201a26441d61fcc9a890c29599b2aa848ea406299d0bb8090ce6027ec82abdd6ec0901000000010000000000000000000000000000000000000000000000000000000000000000ffffffff07044c86041b0146ffffffff0100f2052a01000000434104e18f7afbe4721580e81e8414fc8c24d7cfacf254bb5c7b949450c3e997c2dc1242487a8169507b631eb3771f2b425483fb13102c4eb5d858eef260fe70fbfae0ac00000000010000000196608ccbafa16abada902780da4dc35dafd7af05fa0da08cf833575f8cf9e836000000004a493046022100dab24889213caf43ae6adc41cf1c9396c08240c199f5225acf45416330fd7dbd022100fe37900e0644bf574493a07fc5edba06dbc07c311b947520c2d514bc5725dcb401ffffffff0100f2052a010000001976a914f15d1921f52e4007b146dfa60f369ed2fc393ce288ac000000000100000001fb766c1288458c2bafcfec81e48b24d98ec706de6b8af7c4e3c29419bfacb56d000000008c493046022100f268ba165ce0ad2e6d93f089cfcd3785de5c963bb5ea6b8c1b23f1ce3e517b9f022100da7c0f21adc6c401887f2bfd1922f11d76159cbc597fbd756a23dcbb00f4d7290141042b4e8625a96127826915a5b109852636ad0da753c9e1d5606a50480cd0c40f1f8b8d898235e571fe9357d9ec842bc4bba1827daaf4de06d71844d0057707966affffffff0280969800000000001976a9146963907531db72d0ed1a0cfb471ccb63923446f388ac80d6e34c000000001976a914f0688ba1c0d1ce182c7af6741e02658c7d4dfcd388ac000000000100000002c40297f730dd7b5a99567eb8d27b78758f607507c52292d02d4031895b52f2ff010000008b483045022100f7edfd4b0aac404e5bab4fd3889e0c6c41aa8d0e6fa122316f68eddd0a65013902205b09cc8b2d56e1cd1f7f2fafd60a129ed94504c4ac7bdc67b56fe67512658b3e014104732012cb962afa90d31b25d8fb0e32c94e513ab7a17805c14ca4c3423e18b4fb5d0e676841733cb83abaf975845c9f6f2a8097b7d04f4908b18368d6fc2d68ecffffffffca5065ff9617cbcba45eb23726df6498a9b9cafed4f54cbab9d227b0035ddefb000000008a473044022068010362a13c7f9919fa832b2dee4e788f61f6f5d344a7c2a0da6ae740605658022006d1af525b9a14a35c003b78b72bd59738cd676f845d1ff3fc25049e01003614014104732012cb962afa90d31b25d8fb0e32c94e513ab7a17805c14ca4c3423e18b4fb5d0e676841733cb83abaf975845c9f6f2a8097b7d04f4908b18368d6fc2d68ecffffffff01001ec4110200000043410469ab4181eceb28985b9b4e895c13fa5e68d85761b7eee311db5addef76fa8621865134a221bd01f28ec9999ee3e021e60766e9d1f3458c115fb28650605f11c9ac000000000100000001cdaf2f758e91c514655e2dc50633d1e4c84989f8aa90a0dbc883f0d23ed5c2fa010000008b48304502207ab51be6f12a1962ba0aaaf24a20e0b69b27a94fac5adf45aa7d2d18ffd9236102210086ae728b370e5329eead9accd880d0cb070aea0c96255fae6c4f1ddcce1fd56e014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff02404b4c00000000001976a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac002d3101000000001976a9141befba0cdc1ad56529371864d9f6cb042faa06b588ac000000000100000001b4a47603e71b61bc3326efd90111bf02d2f549b067f4c4a8fa183b57a0f800cb010000008a4730440220177c37f9a505c3f1a1f0ce2da777c339bd8339ffa02c7cb41f0a5804f473c9230220585b25a2ee80eb59292e52b987dad92acb0c64eced92ed9ee105ad153cdb12d001410443bd44f683467e549dae7d20d1d79cbdb6df985c6e9c029c8d0c6cb46cc1a4d3cf7923c5021b27f7a0b562ada113bc85d5fda5a1b41e87fe6e8802817cf69996ffffffff0280651406000000001976a9145505614859643ab7b547cd7f1f5e7e2a12322d3788ac00aa0271000000001976a914ea4720a7a52fc166c55ff2298e07baf70ae67e1b88ac00000000010000000586c62cd602d219bb60edb14a3e204de0705176f9022fe49a538054fb14abb49e010000008c493046022100f2bc2aba2534becbdf062eb993853a42bbbc282083d0daf9b4b585bd401aa8c9022100b1d7fd7ee0b95600db8535bbf331b19eed8d961f7a8e54159c53675d5f69df8c014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff03ad0e58ccdac3df9dc28a218bcf6f1997b0a93306faaa4b3a28ae83447b2179010000008b483045022100be12b2937179da88599e27bb31c3525097a07cdb52422d165b3ca2f2020ffcf702200971b51f853a53d644ebae9ec8f3512e442b1bcb6c315a5b491d119d10624c83014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff2acfcab629bbc8685792603762c921580030ba144af553d271716a95089e107b010000008b483045022100fa579a840ac258871365dd48cd7552f96c8eea69bd00d84f05b283a0dab311e102207e3c0ee9234814cfbb1b659b83671618f45abc1326b9edcc77d552a4f2a805c0014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffffdcdc6023bbc9944a658ddc588e61eacb737ddf0a3cd24f113b5a8634c517fcd2000000008b4830450221008d6df731df5d32267954bd7d2dda2302b74c6c2a6aa5c0ca64ecbabc1af03c75022010e55c571d65da7701ae2da1956c442df81bbf076cdbac25133f99d98a9ed34c014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffffe15557cd5ce258f479dfd6dc6514edf6d7ed5b21fcfa4a038fd69f06b83ac76e010000008b483045022023b3e0ab071eb11de2eb1cc3a67261b866f86bf6867d4558165f7c8c8aca2d86022100dc6e1f53a91de3efe8f63512850811f26284b62f850c70ca73ed5de8771fb451014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff01404b4c00000000001976a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac00000000010000000166d7577163c932b4f9690ca6a80b6e4eb001f0a2fa9023df5595602aae96ed8d000000008a4730440220262b42546302dfb654a229cefc86432b89628ff259dc87edd1154535b16a67e102207b4634c020a97c3e7bbd0d4d19da6aa2269ad9dded4026e896b213d73ca4b63f014104979b82d02226b3a4597523845754d44f13639e3bf2df5e82c6aab2bdc79687368b01b1ab8b19875ae3c90d661a3d0a33161dab29934edeb36aa01976be3baf8affffffff02404b4c00000000001976a9144854e695a02af0aeacb823ccbc272134561e0a1688ac40420f00000000001976a914abee93376d6b37b5c2940655a6fcaf1c8e74237988ac0000000001000000014e3f8ef2e91349a9059cb4f01e54ab2597c1387161d3da89919f7ea6acdbb371010000008c49304602210081f3183471a5ca22307c0800226f3ef9c353069e0773ac76bb580654d56aa523022100d4c56465bdc069060846f4fbf2f6b20520b2a80b08b168b31e66ddb9c694e240014104976c79848e18251612f8940875b2b08d06e6dc73b9840e8860c066b7e87432c477e9a59a453e71e6d76d5fe34058b800a098fc1740ce3012e8fc8a00c96af966ffffffff02c0e1e400000000001976a9144134e75a6fcb6042034aab5e18570cf1f844f54788ac404b4c00000000001976a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac00000000";
}

BOOST_AUTO_TEST_SUITE_END()
