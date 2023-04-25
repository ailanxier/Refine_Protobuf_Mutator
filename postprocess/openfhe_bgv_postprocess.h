#pragma once

#include "post_util.h"
using namespace OpenFHE;

const vector<uint32_t> multiplicativeDepth_range    = {3, 5};
const vector<uint32_t> batchSize_range              = {0, 2048};
const vector<uint32_t> ksTech_range                 = {1, 2};
const KeySwitchTechnique ksTech_default             = KeySwitchTechnique::HYBRID;
const vector<int32_t> preMode_range                 = {0, 3}; // DIVIDE_AND_ROUND_HRA is not supported for BGVRNS
const vector<uint32_t> digitSize_range              = {10, 30};
const vector<uint32_t> smaller_digitSize_range      = {10, 20};
const vector<uint32_t> larger_digitSize_range       = {16, 30};
const vector<float> standardDeviation_range         = {0, 10};
const vector<uint32_t> scalTech_range               = {1, 3};
const vector<uint32_t> firstModSize_range           = {40, 60};
const vector<uint32_t> scalingModSize_range         = {40, 59};
const uint32_t  maxPlaintextModulus                 = 1e5;
const vector<uint32_t> evalAddCount_range           = {0, 20};
const vector<uint32_t> keySwitchCount_range         = {0, (int)1e4};
const vector<uint32_t> multiHopModSize_range        = {0, 60};
uint32_t maxDataLen                                 = 0;
const uint32_t maxRingDimExp                        = 15;
const uint32_t maxDataNum                           = 10;
const uint32_t maxAPINum                            = 10;
const int maxRotateIndexedNum                       = 1;
// const vector<uint32_t> ringDimValue                 = {8192, 16384, 32768};

/**
 * @brief: Prior to testing OpenFHE's CKKS scheme, post-processing is applied to the input protobufs to 
 *         improve input validity and reduce timeout probability through constraints. 
 * @param msg: the input protobuf
 * @param out_buf: the output buffer
 * @param temp: Allocate a dynamic memory space to store the serialized protobuf result.
 */
int PostProcessMessage(Root& msg, unsigned char **out_buf, char *temp){
    auto param = msg.mutable_param();
    auto multiplicativeDepth = 1;
    
// ======================== postprocess parameter ========================
    if(param->has_multiplicativedepth()){
        multiplicativeDepth = clampToRange(param->multiplicativedepth(), multiplicativeDepth_range);
        param->set_multiplicativedepth(multiplicativeDepth);
    }
    // INVALID_KS_TECH will throw exception
    if(param->has_kstech() && param->kstech() == KeySwitchTechnique::INVALID_KS_TECH)
        param->set_kstech((KeySwitchTechnique)GetRandomNum(ksTech_range[0], ksTech_range[1]));
    
    // TEST: BGV use KeySwitchTechnique = BV and encryptionTechnique = EXTENDED crash (to be confirmed)
    if(param->has_kstech() && param->kstech() == KeySwitchTechnique::BV 
                              && param->encryptiontechnique() == EncryptionTechnique::EXTENDED)
        param->set_kstech(KeySwitchTechnique::HYBRID);

    if(param->has_premode() && param->premode() == ProxyReEncryptionMode::DIVIDE_AND_ROUND_HRA)
        param->set_premode((ProxyReEncryptionMode)GetRandomNum(preMode_range[0], preMode_range[1]));
    // TEST: !!! EncryptionTechnique = EXTENDED && PREMode = NOT_SET => crash (to be confirmed)
    if((param->premode() == ProxyReEncryptionMode::NOT_SET) && 
        param->encryptiontechnique() == EncryptionTechnique::EXTENDED)
        param->set_premode((ProxyReEncryptionMode)GetRandomNum(1, preMode_range[1]));

    // PREMode = NOISE_FLOODING_HRA && KeySwitching = HIBIRD => digitSize = 0
    if(param->kstech() == KeySwitchTechnique::HYBRID && param->premode() == ProxyReEncryptionMode::NOISE_FLOODING_HRA)
        param->set_digitsize(0);
    else {
        // TEST: set a larger digitSize for efficiency
        if(param->has_multipartymode() && param->multipartymode() == MultipartyMode::NOISE_FLOODING_MULTIPARTY)
            param->set_digitsize(clampToRange(param->digitsize(), smaller_digitSize_range));
        else if(multiplicativeDepth == multiplicativeDepth_range[1]) 
            param->set_digitsize(clampToRange(param->digitsize(), larger_digitSize_range));
        else
            param->set_digitsize(clampToRange(param->digitsize(), digitSize_range));
    }

    if(param->has_standarddeviation()){
        auto stddev = param->standarddeviation();
        if(stddev > standardDeviation_range[1] || stddev < standardDeviation_range[0])
            param->set_standarddeviation(GetRandomNum(standardDeviation_range[0], standardDeviation_range[1]));
    }
  
    // TEST: set firstmodsize and scalingmodsize to zero for now
    param->set_firstmodsize(0);
    param->set_scalingmodsize(0);

    // TEST:
    param->set_numlargedigits(0);

    // if no data list, set a random data list with length of MAX_NEW_REPEATED_SIZE
    maxDataLen = msg.evaldata().alldatalists_size() == 0 ? MAX_NEW_REPEATED_SIZE : 1;
    for(auto& data : msg.evaldata().alldatalists()) maxDataLen = max(maxDataLen, (uint32_t)data.datalist_size());
    // TEST: securitylevel and ringDim is to be confirmed
    // since rotation is on ringDim / 2, ringDim should be at least 2 * maxDataLen
    auto ringDim = param->ringdim();
    auto minRingDimExp = levelUpToPowerOfTwo(maxDataLen * 2);
    if(ringDim < (1 << minRingDimExp) || ringDim > (1 << maxRingDimExp) || ringDim == 0 || (ringDim & (ringDim - 1)) != 0)
        ringDim = (1 << GetRandomNum(minRingDimExp, maxRingDimExp));
    // TEST: To introduce a small variance to ringDim
    if(GetRandomIndex(2000) == 0) ringDim += GetRandomNum(-2, 2);
    // TEST: manually set ringDim
    param->set_ringdim(ringDim);
    param->set_securitylevel(SecurityLevel::HEStd_NotSet);    

    if(param->has_batchsize()){
        //  Only be set to zero or a power of two.
        auto size = param->batchsize();
        vector<uint32_t> temp_range = {(uint32_t)1 << levelUpToPowerOfTwo(maxDataLen), 
                                        (uint32_t)1 << levelUpToPowerOfTwo(reduceToPowerOfTwo(ringDim))};
        size = clampToRange(size, temp_range);
        if(GetRandomIndex(3) == 0)  size = 0;
        param->set_batchsize(reduceToPowerOfTwo(size));
    }  
    // TEST: plaintextModulus(P) must comply with (P-1)/m is an integer, where m is (cyclotomic = 2 * ringDim)
    auto cyclotomic = 2 * ringDim;
    auto plaintextModulus = param->plaintextmodulus();
    if((plaintextModulus - 1) % cyclotomic != 0 || !Miller_Rabin(plaintextModulus)){
        plaintextModulus = cyclotomic + 1;
        vector<uint32_t> temp_ptm;
        while(plaintextModulus < maxPlaintextModulus){
            if(Miller_Rabin(plaintextModulus))
                temp_ptm.push_back(plaintextModulus);
            plaintextModulus += cyclotomic;
        }
        // TEST: do not found a valid plaintextModulus
        if(temp_ptm.size() == 0) plaintextModulus = GetRandomNum(cyclotomic + 1, maxPlaintextModulus);
        else
            plaintextModulus = temp_ptm[GetRandomIndex(temp_ptm.size() - 1)];
    }
    // TEST: To introduce a small variance to plaintextModulus
    if(GetRandomIndex(500) == 0) plaintextModulus += GetRandomNum(-1, 1);
    param->set_plaintextmodulus(plaintextModulus);

    vector<int32_t> evalData_range(2);
    evalData_range[0] = -(int)plaintextModulus;
    evalData_range[1] = plaintextModulus;

    if(param->has_evaladdcount())
        param->set_evaladdcount(clampToRange(param->evaladdcount(), evalAddCount_range));
    if(param->has_keyswitchcount())
        param->set_keyswitchcount(clampToRange(param->keyswitchcount(), keySwitchCount_range));
    param->set_multihopmodsize(clampToRange(param->multihopmodsize(), multiHopModSize_range));

// ======================== Postprocess EvalData ========================   
    // TEST: reduce the number of data to be evaluated for efficiency
    auto evalData = msg.mutable_evaldata()->mutable_alldatalists();
    while(evalData->size() > maxDataNum){
        int tmp = MAX_BINARY_INPUT_SIZE;
        auto field = msg.evaldata().GetDescriptor()->FindFieldByName("allDataLists");
        DeleteRepeatedField(msg.mutable_evaldata(), field, tmp);
    }
    dataNum = max(1, msg.evaldata().alldatalists_size());    
    // at least one data list
    for(auto& dataList : *evalData){
        for(auto& data : *dataList.mutable_datalist())
            data = clampToRange(data, evalData_range);
        if(dataList.datalist_size() == 0) 
            dataList.add_datalist(GetRandomNum(evalData_range[0], evalData_range[1]));
    }
    // at least one dataList
    if(msg.evaldata().alldatalists().size() == 0){
        auto dataList = msg.mutable_evaldata()->add_alldatalists();
        for(int i = 0; i < MAX_NEW_REPEATED_SIZE; i++)
            dataList->add_datalist(GetRandomNum(evalData_range[0], evalData_range[1]));
    }

// ======================== Postprocess APISequence ========================  
    // RotateIndexes: clamp the index and src here, then add unset index
    map<int, bool> indexMap;
    auto apiList = msg.mutable_apisequence()->mutable_apilist();
    while(apiList->size() > maxAPINum){
        int tmp = MAX_BINARY_INPUT_SIZE;
        auto field = msg.apisequence().GetDescriptor()->FindFieldByName("apiList");
        DeleteRepeatedField(msg.mutable_apisequence(), field, tmp);
    }
    for(auto& api : *apiList) {
        if(api.has_rotateonelist()) {
            auto index = api.rotateonelist().index();
            auto src = apiSrcAndDstClampToRange(api.mutable_rotateonelist()->src());
            api.mutable_rotateonelist()->set_src(src);
            auto len = msg.evaldata().alldatalists()[src].datalist_size();
            // TEST: index is in range [-len, len]
            index = clampToRange(index, {-len + 1, len - 1});
            api.mutable_rotateonelist()->set_index(index);
            indexMap[index] = true;
        }
    }
    // TEST: random delete repeated field to satisfy the size limit
    while(param->rotateindexes_size() > maxRotateIndexedNum)
        param->mutable_rotateindexes()->RemoveLast();
    for(auto& index : *param->mutable_rotateindexes()){
        index = clampToRange(index, {-(int)ringDim / 2 + 1, (int)ringDim / 2 - 1});
        if(indexMap.find(index) != indexMap.end())
            indexMap[index] = false;
    }
    // XXX: have probability to not set index
    for(auto& index : indexMap)
        if(index.second && GetRandomIndex(2000))
            param->add_rotateindexes(index.first);
    for(auto & api : *apiList){
        auto dst = api.dst();
        api.set_dst(apiSrcAndDstClampToRange(dst));
        if(api.has_addtwolist()){
            api.mutable_addtwolist()->set_src1(apiSrcAndDstClampToRange(api.mutable_addtwolist()->src1()));
            api.mutable_addtwolist()->set_src2(apiSrcAndDstClampToRange(api.mutable_addtwolist()->src2()));
        }else if(api.has_addmanylist()){
            for(auto& src : *api.mutable_addmanylist()->mutable_srcs())
                src = apiSrcAndDstClampToRange(src);
            if(api.addmanylist().srcs_size() == 0)
                api.mutable_addmanylist()->add_srcs(GetRandomIndex(dataNum - 1));
        }else if(api.has_subtwolist()){
            api.mutable_subtwolist()->set_src1(apiSrcAndDstClampToRange(api.mutable_subtwolist()->src1()));
            api.mutable_subtwolist()->set_src2(apiSrcAndDstClampToRange(api.mutable_subtwolist()->src2()));
        }else if(api.has_multwolist()){
            api.mutable_multwolist()->set_src1(apiSrcAndDstClampToRange(api.mutable_multwolist()->src1()));
            api.mutable_multwolist()->set_src2(apiSrcAndDstClampToRange(api.mutable_multwolist()->src2())); 
        }else if(api.has_mulmanylist()){
              // XXX: Since mulmanylist always cause segfault, we only use it in MaxMultiplicativeDepth
            if(multiplicativeDepth != multiplicativeDepth_range[1])
                while(api.mutable_mulmanylist()->srcs_size() > 0)
                    api.mutable_mulmanylist()->mutable_srcs()->RemoveLast();
            for(auto& src : *api.mutable_mulmanylist()->mutable_srcs())
                src = apiSrcAndDstClampToRange(src);
            if(api.mulmanylist().srcs_size() == 0)
                api.mutable_mulmanylist()->add_srcs(GetRandomIndex(dataNum - 1));
        }
    }
    static int index = 1;
    index++;
    string buffer = msg.DebugString();
    ofstream of("proto_bout.txt", std::ios::trunc);
    of << "================"<< index <<"================"<< endl;
    of << buffer;of.close();
    // write to out_buf
    buffer = msg.SerializeAsString();
    // strcpy doesn't work, because it will stop at every '\0' in buffer
    for(int i = 0;i < buffer.size();i++)
        temp[i] = buffer[i];
    temp[buffer.size()] = '\0';
    *out_buf = (unsigned char *)temp;
    return buffer.size();
}