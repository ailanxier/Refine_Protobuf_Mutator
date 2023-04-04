#include <iostream>
#include <fstream>
#include <stddef.h>
#include "fhe.pb.h"
#include "proto_text_seed.h"
#include "google/protobuf/text_format.h"

using namespace std;
using namespace google::protobuf;
#define COUT_RED "\033[31m"
#define COUT_GREEN "\033[32m"
#define COUT_END_COLOR "\033[0m"
inline void print_words(const vector<string> words, int highlight_pos = 0){
    int i = 1;
    for(const auto& word : words){
        if(i == highlight_pos)
            cout << COUT_GREEN << word << COUT_END_COLOR << " ";
        else 
            cout << word << " ";
        i++;
    }
    cout << endl;
}
int main(int argc ,char**argv){
    TestRootMsg msg;
    int i = 0;
    for(int i = 0;i < SeedNum;i++){
        auto param = msg.mutable_param();
        auto evalData = msg.mutable_evaldata();
        auto apiSeq = msg.mutable_apisequence();
        param->set_num32(num32[i]);
        param->set_num64(num64[i]);
        param->set_secretkeydist(skd[i]);
        
        EvalData::OneDataList oneDataList;
        for(auto & dataList : allData[i]){
            oneDataList.clear_datalist();
            for(auto & data : dataList)
                oneDataList.add_datalist(data);
            evalData->add_alldatalists()->CopyFrom(oneDataList);
        }
        
        APISequence::OneAPI oneAPI;
        for(auto it : apiList[i]){
            if(auto* a2p = dynamic_cast<AddTwoList*>(it)){
                APISequence::OneAPI::AddTwoList addTwoList;
                addTwoList.set_src1(a2p->src1);
                addTwoList.set_src2(a2p->src2);
                oneAPI.mutable_addtwolist()->CopyFrom(addTwoList);
                oneAPI.set_dst(a2p->dst);
                apiSeq->add_apilist()->CopyFrom(oneAPI);
            }else if(auto* ap = dynamic_cast<AddManyList*>(it)){
                APISequence::OneAPI::AddManyList addManyList;
                for(auto & src : ap->srcs)
                    addManyList.add_srcs(src);
                oneAPI.mutable_addmanylist()->CopyFrom(addManyList);
                oneAPI.set_dst(ap->dst);
                apiSeq->add_apilist()->CopyFrom(oneAPI);
            }else if(auto* m2p = dynamic_cast<MulTwoList*>(it)){
                APISequence::OneAPI::MulTwoList mulTwoList;
                mulTwoList.set_src1(m2p->src1);
                mulTwoList.set_src2(m2p->src2);
                oneAPI.mutable_multwolist()->CopyFrom(mulTwoList);
                oneAPI.set_dst(m2p->dst);
                apiSeq->add_apilist()->CopyFrom(oneAPI);
            }else if(auto* mp = dynamic_cast<MulManyList*>(it)){
                APISequence::OneAPI::MulManyList mulManyList;
                for(auto & src : mp->srcs)
                    mulManyList.add_srcs(src);
                oneAPI.mutable_mulmanylist()->CopyFrom(mulManyList);
                oneAPI.set_dst(mp->dst);
                apiSeq->add_apilist()->CopyFrom(oneAPI);
            }else if(auto* sp = dynamic_cast<ShiftOneList*>(it)){
                APISequence::OneAPI::ShiftOneList shiftOneList;
                shiftOneList.set_src(sp->src);
                shiftOneList.set_index(sp->index);
                oneAPI.mutable_shiftonelist()->CopyFrom(shiftOneList);
                oneAPI.set_dst(sp->dst);
                apiSeq->add_apilist()->CopyFrom(oneAPI);
            }
        }
        print_words({"==============", to_string(i), "=============="}, 2);
        ofstream OutFile(to_string(i)+".txt", ios::out); 
        msg.SerializePartialToOstream(&OutFile);
        msg.PrintDebugString();
        msg.Clear();
    }
    return 0;
}
