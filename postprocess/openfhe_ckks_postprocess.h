#ifndef OPENFHE_CKKS_POSTPROCESS_H_
#define OPENFHE_CKKS_POSTPROCESS_H_
#include "protobuf_mutator/mutator.h"
#include "proto/proto_setting.h"
using namespace std;
using namespace protobuf_mutator;
using namespace OpenFHE;

const vector<uint32_t> multiplicativeDepth_range = {1, 5};
const vector<uint32_t> batchSize_range = {8, 2048};
const vector<uint32_t> ksTech_range = {1, 2};
const KeySwitchTechnique ksTech_default = KeySwitchTechnique::HYBRID;
const vector<uint32_t> preMode_range = {0, 1}; // only INDCPA and NotSet are supported for CKKS
const vector<uint32_t> digitSize_range = {10, 30};
const vector<uint32_t> larger_digitSize_range = {16, 30};
const vector<float> standardDeviation_range = {-1e8, 1e8};
const vector<uint32_t> scalTech_range = {1, 3};
const vector<uint32_t> firstModSize_range = {40, 60};
const vector<uint32_t> scalingModSize_range = {40, 59};
const vector<uint32_t> numLargeDigits_range = {0, 3};
const vector<uint32_t> securityLevel_range = {0, 1};
const vector<int32_t> rotateIndex_range = {(int)-2e4, (int)2e4};
const int rotateIndexed_maxNum = 1;
const vector<double> evalData_range = {-1, 1};
uint32_t dataNum = 0;

/**
 * @brief limit value into [range[0], range[1]]
 */
template<typename T>
inline typename std::enable_if<std::is_integral<T>::value, T>::type 
clampToRange(const T value, const vector<T>& range) {
    assert(range[0] <= range[1]);
    if(range[0] <= value && value <= range[1])
        return value;
    T len = range[1] - range[0] + 1;
    // constrain value to [0, len-1] and then shift this interval by range[0].
    return NotNegMod(value, len) + range[0];
}

template<typename T>
inline typename std::enable_if<std::is_floating_point<T>::value, T>::type
clampToRange(const T value, const vector<T>& range) {
    assert(range[0] <= range[1]);
    if(range[0] <= value && value <= range[1])
        return value;
    T len = range[1] - range[0];
    T integer = floor(value);
    T decimal = value - integer;
    return fmod(fmod(integer, len) + len, len) + range[0] + decimal;
}

/**
 * @brief Reduce a uint32_t value to the largest power of 2 that is less than or equal to it
 * @param value the value to be reduced
 */
inline uint32_t reduceToPowerOfTwo(uint32_t value) {
    if((value & (value - 1)) == 0) return value;
    uint32_t res = 1;
    while (res <= value) {
        auto temp = res;
        res <<= 1;
        // If an overflow occurs, the previous value is the desired maximum power of 2.
        if(res < temp) return temp;
    }
    return res >> 1;
}

/**
 * @brief wrapper for clampToRange(), limit value into [0, dataNum - 1]
 */
template<typename T>
inline typename std::enable_if<std::is_integral<T>::value, T>::type 
apiSrcAndDstClampToRange(const T value) {
    // If there is no data to be computed, the API will not be invoked.
    if(dataNum == 0) return value;
    return clampToRange(value, {0, dataNum - 1});
}

/**
 * @brief: Prior to testing OpenFHE's CKKS scheme, post-processing is applied to the input protobufs to 
 *         improve input validity and reduce timeout probability through constraints. 
 * @param msg: the input protobuf
 * @param out_buf: the output buffer
 * @param temp: Allocate a dynamic memory space to store the serialized protobuf result.
 */
int PostProcessMessage(Root& msg, unsigned char **out_buf, char *temp){
    static int index = 1;
    index++;
    string buffer;

    auto param = msg.mutable_param();

// ======================== postprocess parameter ========================
    if(param->has_multiplicativedepth()){
        auto depth = param->multiplicativedepth();
        param->set_multiplicativedepth(clampToRange(depth, multiplicativeDepth_range));
    }
    if(param->has_batchsize()){
        //  Only be set to zero or a power of two.
        auto size = param->batchsize();
        size = clampToRange(size, batchSize_range);
        if(GetRandomIndex(3) == 0)  size = 0;
        param->set_batchsize(reduceToPowerOfTwo(size));
    }
    // INVALID_KS_TECH will throw exception
    if(param->has_kstech() && param->kstech() == KeySwitchTechnique::INVALID_KS_TECH)
        param->set_kstech((KeySwitchTechnique)GetRandomNum(ksTech_range[0], ksTech_range[1]));
    
    if(param->has_premode()){
        // only INDCPA and NotSet are supported for CKKS
        auto pre = param->premode();
        param->set_premode((ProxyReEncryptionMode)clampToRange((uint32_t)pre, preMode_range));
    }
    // PREMode = NOISE_FLOODING_HRA && KeySwitching = HIBIRD => digitSize = 0
    if(param->kstech() == KeySwitchTechnique::HYBRID && param->premode() == ProxyReEncryptionMode::NOISE_FLOODING_HRA)
        param->set_digitsize(0);
    else {
        auto depth = param->has_multiplicativedepth() ? param->multiplicativedepth() : 1;
        // TEST: set a larger digitSize for efficiency
        if(depth == multiplicativeDepth_range[1]) 
            param->set_digitsize(clampToRange(param->digitsize(), larger_digitSize_range));
        else
            param->set_digitsize(clampToRange(param->digitsize(), digitSize_range));
    }

    if(param->has_standarddeviation()){
        auto stddev = param->standarddeviation();
        param->set_standarddeviation(clampToRange(stddev, standardDeviation_range));
    }
    // INVALID_RS_TECHNIQUE will throw exception
    if(param->has_scaltech())
        param->set_scaltech((ScalingTechnique)clampToRange((uint32_t)param->scaltech(), scalTech_range));
    
    if(param->has_firstmodsize()){
        auto size = param->firstmodsize();
        param->set_firstmodsize(clampToRange(size, firstModSize_range));
    }

    if(param->has_scalingmodsize()){
        auto size = clampToRange(param->scalingmodsize(), scalingModSize_range);
        // FirstModSize can't be equal to scalingModSize (otherwise exception will be thrown)
        if(size == param->firstmodsize()) size = clampToRange(size - 1, scalingModSize_range);
        param->set_scalingmodsize(size);
    }

    // TEST:
    param->set_numlargedigits(0);

    // TEST: securitylevel is to be confirmed, do not set to HEStd_NotSet and 256(too slow) for now
    if(param->securitylevel() == SecurityLevel::HEStd_NotSet || 
        param->securitylevel() == SecurityLevel::HEStd_256_classic)
        param->set_securitylevel((SecurityLevel)GetRandomNum(securityLevel_range[0], securityLevel_range[1]));
    // ringdim is to be confirmed, set to zero for now
    param->set_ringdim(0);

    // RotateIndexes: check range, add unset index
    map<int, bool> indexMap;
    auto apiList = msg.mutable_apisequence()->mutable_apilist();
    for(auto& api : *apiList) {
        if(api.has_rotateonelist()) {
            auto index = api.rotateonelist().index();
            index = clampToRange(index, rotateIndex_range);
            api.mutable_rotateonelist()->set_index(index);
            indexMap[index] = true;
        }
    }
    auto rotateIndexField = param->GetDescriptor()->FindFieldByName("rotateIndexes");
    int remainSizeForRotateIndex = MAX_BINARY_INPUT_SIZE;
    // TEST: random delete repeated field to satisfy the size limit
    while(param->rotateindexes_size() > rotateIndexed_maxNum)
        param->mutable_rotateindexes()->RemoveLast();
    for(auto& index : *param->mutable_rotateindexes()){
        index = clampToRange(index, rotateIndex_range);
        if(indexMap.find(index) != indexMap.end())
            indexMap[index] = false;
    }
    for(auto& index : indexMap)
        if(index.second && GetRandomIndex(200))
            param->add_rotateindexes(index.first);
    
    param->set_encryptiontechnique(STANDARD);

// ======================== Postprocess EvalData ========================   
    auto evalData = msg.mutable_evaldata()->mutable_alldatalists();
    for(auto& dataList : *evalData)
        for(auto& data : *dataList.mutable_datalist())
            data = clampToRange(data, evalData_range);
    if(msg.evaldata().alldatalists().size() == 0){
        auto dataList = msg.mutable_evaldata()->add_alldatalists();
        for(int i = 0; i < MAX_NEW_REPEATED_SIZE; i++)
            dataList->add_datalist(GetRandomNum(evalData_range[0], evalData_range[1]));
    }

// ======================== Postprocess APISequence ========================  
    dataNum = msg.evaldata().alldatalists_size();
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
            for(auto& src : *api.mutable_mulmanylist()->mutable_srcs())
                src = apiSrcAndDstClampToRange(src);
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
    buffer = msg.DebugString();
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