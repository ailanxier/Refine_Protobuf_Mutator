#ifndef OPENFHE_CKKS_POSTPROCESS_H_
#define OPENFHE_CKKS_POSTPROCESS_H_
#include "post_util.h"
using namespace OpenFHE;
const vector<uint32_t> multiplicativeDepth_range    = {1, 5};
const vector<uint32_t> batchSize_range              = {8, 2048};
const vector<uint32_t> ksTech_range                 = {1, 2};
const KeySwitchTechnique ksTech_default             = KeySwitchTechnique::HYBRID;
const vector<uint32_t> preMode_range                = {0, 1}; // only INDCPA and NotSet are supported for CKKS
const vector<uint32_t> digitSize_range              = {10, 30};
const vector<uint32_t> smaller_digitSize_range      = {10, 20};
const vector<uint32_t> larger_digitSize_range       = {16, 30};
const vector<float> standardDeviation_range         = {0, 1e4};
const vector<uint32_t> scalTech_range               = {1, 3};
const vector<uint32_t> firstModSize_range           = {40, 60};
const vector<uint32_t> smaller_firstModSize_range   = {40, 51};
const vector<uint32_t> scalingModSize_range         = {40, 59};
const vector<uint32_t> smaller_scalingModSize_range = {40, 50};
const vector<uint32_t> ringDimValue                 = {8192, 16384, 32768};
const vector<uint32_t> securityLevel_range          = {0, 1};
const vector<int32_t> rotateIndex_range             = {(int)-2e4, (int)2e4};
const int rotateIndexed_maxNum                      = 1;
const vector<double> evalData_range                 = {-1, 1};
const uint32_t maxRingDimExp                        = 14;
uint32_t maxDataLen                                 = 0;
const uint32_t maxDataNum                           = 10;
const uint32_t maxAPINum                            = 10;
// const vector<uint32_t> numLargeDigits_range         = {0, 3};

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
    // TEST: CKKS use KeySwitchTechnique = BV and encryptionTechnique = EXTENDED crash (to be confirmed)
    if(param->has_kstech() && param->kstech() == KeySwitchTechnique::INVALID_KS_TECH)
        param->set_kstech((KeySwitchTechnique)GetRandomNum(ksTech_range[0], ksTech_range[1]));
        
    if(param->has_kstech() && param->kstech() == KeySwitchTechnique::BV 
                              && param->encryptiontechnique() == EncryptionTechnique::EXTENDED)
        param->set_kstech(KeySwitchTechnique::HYBRID);
    

    if(param->has_premode()){
        param->set_premode((ProxyReEncryptionMode)clampToRange((uint32_t)param->premode(), preMode_range));
        // TEST: !!! EncryptionTechnique = EXTENDED && PREMode = NOT_SET => crash (to be confirmed)
        if(param->premode() == ProxyReEncryptionMode::NOT_SET && param->encryptiontechnique() == EncryptionTechnique::EXTENDED)
            param->set_premode(ProxyReEncryptionMode::INDCPA);
    }

    // PREMode = NOISE_FLOODING_HRA && KeySwitching = HIBIRD => digitSize = 0
    if(param->kstech() == KeySwitchTechnique::HYBRID && param->premode() == ProxyReEncryptionMode::NOISE_FLOODING_HRA)
        param->set_digitsize(0);
    else {
        auto depth = param->has_multiplicativedepth() ? param->multiplicativedepth() : 1;
        // TEST: set a larger digitSize for efficiency
        if(param->has_multipartymode() && param->multipartymode() == MultipartyMode::NOISE_FLOODING_MULTIPARTY)
            param->set_digitsize(clampToRange(param->digitsize(), smaller_digitSize_range));
        else if(depth == multiplicativeDepth_range[1]) 
            param->set_digitsize(clampToRange(param->digitsize(), larger_digitSize_range));
        else
            param->set_digitsize(clampToRange(param->digitsize(), digitSize_range));
    }

    if(param->has_standarddeviation()){
        auto stddev = param->standarddeviation();
        if(stddev > standardDeviation_range[1] || stddev < standardDeviation_range[0])
            param->set_standarddeviation(GetRandomNum(standardDeviation_range[0], standardDeviation_range[1]));
    }

    // INVALID_RS_TECHNIQUE will throw exception
    if(param->has_scaltech())
        param->set_scaltech((ScalingTechnique)clampToRange((uint32_t)param->scaltech(), scalTech_range));
    
    if(param->has_firstmodsize()){
        auto size = param->firstmodsize();
        if(multiplicativeDepth == multiplicativeDepth_range[1])
            param->set_firstmodsize(clampToRange(size, smaller_firstModSize_range));
        else
            param->set_firstmodsize(clampToRange(size, firstModSize_range));
    }

    if(param->has_scalingmodsize()){
        auto size = param->scalingmodsize();
        if(multiplicativeDepth == multiplicativeDepth_range[1])
            size = clampToRange(size, smaller_scalingModSize_range);
        else
            size = clampToRange(size, scalingModSize_range);
        // FirstModSize can't be equal to scalingModSize (otherwise exception will be thrown)
        if(size == param->firstmodsize()) size = clampToRange(size - 1, scalingModSize_range);
        param->set_scalingmodsize(size); 
    }

    // TEST:
    param->set_numlargedigits(0);

// ======================== Postprocess EvalData ========================   
    maxDataLen = 0;
    auto evalData = msg.mutable_evaldata()->mutable_alldatalists();
    // TEST: reduce the number of data to be evaluated for efficiency
    while(evalData->size() > maxDataNum){
        int tmp = MAX_BINARY_INPUT_SIZE;
        auto field = msg.evaldata().GetDescriptor()->FindFieldByName("allDataLists");
        DeleteRepeatedField(msg.mutable_evaldata(), field, tmp);
    }
    dataNum = max(1, msg.evaldata().alldatalists_size());
    for(auto& dataList : *evalData){
        for(auto& data : *dataList.mutable_datalist())
            data = clampToRange(data, evalData_range);
        if(dataList.datalist_size() == 0) 
            dataList.add_datalist(GetRandomNum(evalData_range[0], evalData_range[1]));
        maxDataLen = max(maxDataLen, (uint32_t)dataList.datalist_size());
    }
    if(msg.evaldata().alldatalists().size() == 0){
        auto dataList = msg.mutable_evaldata()->add_alldatalists();
        for(int i = 0; i < MAX_NEW_REPEATED_SIZE; i++)
            dataList->add_datalist(GetRandomNum(evalData_range[0], evalData_range[1]));
        maxDataLen = max(maxDataLen, (uint32_t)dataList->datalist_size());
    }

    // TEST: ringDim is to be confirmed
    auto ringDim = param->ringdim();
    auto minRingDimExp = levelUpToPowerOfTwo(maxDataLen * 2);
    if(ringDim < (1 << minRingDimExp) || ringDim > (1 << maxRingDimExp) || ringDim == 0 
        || (ringDim & (ringDim - 1)) != 0)
        ringDim = (1 << GetRandomNum(minRingDimExp, maxRingDimExp));

    // TEST: To introduce a small variance to ringDim
    if(GetRandomIndex(1000) == 0) ringDim += GetRandomNum(-2, 2);
    // TEST: securitylevel is to be confirmed, set to HEStd_NotSet for now
    // if(param->securitylevel() == SecurityLevel::HEStd_256_classic)
    //     param->set_securitylevel((SecurityLevel)GetRandomNum(securityLevel_range[0], securityLevel_range[1]));
    
    // if(param->securitylevel() == SecurityLevel::HEStd_NotSet){
    //     param->set_ringdim(ringDim);
    // }else{
    //     param->set_ringdim(0);
    //     ringDim = ringDimValue[GetRandomIndex(ringDimValue.size() - 1)];
    // }
    param->set_ringdim(ringDim);
    param->set_securitylevel(SecurityLevel::HEStd_NotSet);

    if(param->has_batchsize()){
        //  Only be set to zero or a power of two.
        auto size = param->batchsize();
        vector<uint32_t> temp_range = {(uint32_t)1 << levelUpToPowerOfTwo(maxDataLen), 
                                        (uint32_t)1 << levelUpToPowerOfTwo(reduceToPowerOfTwo(ringDim / 2))};

        size = clampToRange(size, temp_range);
        if(GetRandomIndex(3) == 0)  size = 0;
        param->set_batchsize(reduceToPowerOfTwo(size));
    }    
    // TEST: reduce the number of indexes to be rotated for efficiency
    auto apiList = msg.mutable_apisequence()->mutable_apilist();
    while(apiList->size() > maxAPINum){
        int tmp = MAX_BINARY_INPUT_SIZE;
        auto field = msg.apisequence().GetDescriptor()->FindFieldByName("apiList");
        DeleteRepeatedField(msg.mutable_apisequence(), field, tmp);
    }
    // RotateIndexes: check range, add unset index
    map<int, bool> indexMap;
    for(auto& api : *apiList) {
        if(api.has_rotateonelist()) {
            auto index = api.rotateonelist().index();
            index = clampToRange(index, rotateIndex_range);
            api.mutable_rotateonelist()->set_index(index);
            indexMap[index] = true;
        }
    }

    // TEST: random delete repeated field to satisfy the size limit
    while(param->rotateindexes_size() > rotateIndexed_maxNum)
        param->mutable_rotateindexes()->RemoveLast();
    for(auto& index : *param->mutable_rotateindexes()){
        index = clampToRange(index, rotateIndex_range);
        if(indexMap.find(index) != indexMap.end())
            indexMap[index] = false;
    }
    for(auto& index : indexMap)
        if(index.second && GetRandomIndex(1000))
            param->add_rotateindexes(index.first);
    // param->set_encryptiontechnique(STANDARD);

// ======================== Postprocess APISequence ========================  
    for(auto & api : *apiList){
        auto dst = api.dst();
        api.set_dst(apiSrcAndDstClampToRange(dst));
        if(api.has_addtwolist()){
            api.mutable_addtwolist()->set_src1(apiSrcAndDstClampToRange(api.mutable_addtwolist()->src1()));
            api.mutable_addtwolist()->set_src2(apiSrcAndDstClampToRange(api.mutable_addtwolist()->src2()));
        }else if(api.has_addconstant()){
            api.mutable_addconstant()->set_src(apiSrcAndDstClampToRange(api.mutable_addconstant()->src()));
            api.mutable_addconstant()->set_num(clampToRange(api.mutable_addconstant()->num(), evalData_range));
        }else if(api.has_addmanylist()){
            for(auto& src : *api.mutable_addmanylist()->mutable_srcs())
                src = apiSrcAndDstClampToRange(src);
            if(api.addmanylist().srcs_size() == 0)
                api.mutable_addmanylist()->add_srcs(GetRandomIndex(dataNum - 1));
        }else if(api.has_subtwolist()){
            api.mutable_subtwolist()->set_src1(apiSrcAndDstClampToRange(api.mutable_subtwolist()->src1()));
            api.mutable_subtwolist()->set_src2(apiSrcAndDstClampToRange(api.mutable_subtwolist()->src2()));
        }else if(api.has_subconstant()){
            api.mutable_subconstant()->set_src(apiSrcAndDstClampToRange(api.mutable_subconstant()->src()));
            api.mutable_subconstant()->set_num(clampToRange(api.mutable_subconstant()->num(), evalData_range));
        }else if(api.has_multwolist()){
            api.mutable_multwolist()->set_src1(apiSrcAndDstClampToRange(api.mutable_multwolist()->src1()));
            api.mutable_multwolist()->set_src2(apiSrcAndDstClampToRange(api.mutable_multwolist()->src2()));        }else if(api.has_mulconstant()){
            api.mutable_mulconstant()->set_src(apiSrcAndDstClampToRange(api.mutable_mulconstant()->src()));
            api.mutable_mulconstant()->set_num(clampToRange(api.mutable_mulconstant()->num(), evalData_range));
        }else if(api.has_mulmanylist()){
            // TEST: Since mulmanylist always cause segfault, we only use it in MaxMultiplicativeDepth
            if(multiplicativeDepth != multiplicativeDepth_range[1]){
                while(api.mutable_mulmanylist()->srcs_size() > 0)
                    api.mutable_mulmanylist()->mutable_srcs()->RemoveLast();
            }
            for(auto& src : *api.mutable_mulmanylist()->mutable_srcs())
                src = apiSrcAndDstClampToRange(src);
            if(api.mulmanylist().srcs_size() == 0)
                api.mutable_mulmanylist()->add_srcs(GetRandomIndex(dataNum - 1));
        }else if(api.has_linearweightedsum()){
            for(auto& src : *api.mutable_linearweightedsum()->mutable_srcs())
                src = apiSrcAndDstClampToRange(src);
            for(auto& weight : *api.mutable_linearweightedsum()->mutable_weights())
                weight = clampToRange(weight, evalData_range);
            auto maxLen = max(api.mutable_linearweightedsum()->srcs_size(), api.mutable_linearweightedsum()->weights_size());
            while(api.mutable_linearweightedsum()->srcs_size() < maxLen)
                api.mutable_linearweightedsum()->add_srcs(GetRandomIndex(dataNum - 1));
            while(api.mutable_linearweightedsum()->weights_size() < maxLen)
                api.mutable_linearweightedsum()->add_weights(GetRandomNum(evalData_range[0], evalData_range[1]));
        }else if(api.has_rotateonelist())
            api.mutable_rotateonelist()->set_src(apiSrcAndDstClampToRange(api.mutable_rotateonelist()->src()));
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

#endif