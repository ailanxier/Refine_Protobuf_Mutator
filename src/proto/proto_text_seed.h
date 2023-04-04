#include "fhe.pb.h"

using namespace std;
using namespace google::protobuf;

enum cppSecretKeyDist {
    GAUSSIAN,
    UNIFORM_TERNARY,
    SPARSE_TERNARY
};

const int SeedNum = 3;
const int32 num32[SeedNum] = {100, 123456, 3333333};
const int64 num64[SeedNum] = {(1L<<45), 11111111111111111, (int64)1e9+7};
const Param::SecretKeyDist skd[SeedNum] = {(Param::SecretKeyDist)GAUSSIAN, (Param::SecretKeyDist)UNIFORM_TERNARY, (Param::SecretKeyDist)SPARSE_TERNARY};

const uint32 len[SeedNum] = {7, 13, 16};
const uint32 num[SeedNum] = {8, 5, 3};

vector<vector<vector<double>>> allData = {{
    {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7},
    {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7},
    {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7},
    {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7},
    {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7},
    {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7},
    {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7},
    {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7}},
    {
    {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.10, 11.11, 12.12, 13.13},
    {11.1, 12.2, 13.3, 14.4, 15.5, 16.6, 17.7, 18.8, 19.9, 20.10, 21.21, 22.22, 23.23},
    {21.1, 22.2, 23.3, 24.4, 25.5, 26.6, 27.7, 28.8, 29.9, 30.10, 31.11, 32.12, 33.13},
    {31.1, 32.2, 33.3, 34.4, 35.5, 36.6, 37.7, 38.8, 39.9, 40.10, 41.11, 42.12, 43.13},
    {41.1, 42.2, 43.3, 44.4, 45.5, 46.6, 47.7, 48.8, 49.9, 50.10, 51.11, 52.12, 53.13}},
    {
    {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.10, 11.11, 12.12, 13.13, 14.14, 15.15, 16.16},
    {21.1, 22.2, 23.3, 24.4, 25.5, 26.6, 27.7, 28.8, 29.9, 30.10, 31.11, 32.12, 33.13, 34.14, 35.15, 36.16},
    {31.1, 32.2, 33.3, 34.4, 35.5, 36.6, 37.7, 38.8, 39.9, 40.10, 41.11, 42.12, 43.13, 44.14, 45.15, 46.16}}
};

class ApiBase {
public:
    virtual ~ApiBase() = default;
    ApiBase(uint32_t d) : dst(d) {};
    uint32_t dst;
};

class AddTwoList: public ApiBase {
public:
    AddTwoList(uint32_t s1, uint32_t s2, uint32_t d) : ApiBase(d), src1(s1), src2(s2) {};
    uint32_t src1;
    uint32_t src2;
};

class AddManyList: public ApiBase {
public:
    AddManyList(vector<uint32_t> s, uint32_t d) : ApiBase(d), srcs(std::move(s)) {};
    vector<uint32_t> srcs;
};

class MulTwoList: public ApiBase {
public:
    MulTwoList(uint32_t s1, uint32_t s2, uint32_t d) : ApiBase(d), src1(s1), src2(s2) {};
    uint32_t src1;
    uint32_t src2;
};

class MulManyList: public ApiBase {
public:
    MulManyList(vector<uint32_t> s, uint32_t d) : ApiBase(d), srcs(std::move(s)) {};
    vector<uint32_t> srcs;
};

class ShiftOneList: public ApiBase {
public:
    ShiftOneList(uint32_t s, uint32_t i, uint32_t d) : ApiBase(d), src(s), index(i) {};
    uint32_t src;
    uint32_t index;
};

vector<ApiBase*> apiList[SeedNum] = {
    {
        new AddManyList({1, 2, 3}, 0),
        new AddTwoList(3, 4, 5),
        new MulManyList({6, 7, 0, 1}, 2),
        new MulTwoList(7, 2, 4),
        new ShiftOneList(7, 2, 3),
        new AddTwoList(3, 6, 2),
        new MulManyList({0, 1, 2, 3, 4, 5, 6, 7}, 4)},
    {
        new MulManyList({2, 1, 0}, 2),
        new AddManyList({0, 1, 2, 3, 4}, 0),
        new AddTwoList(4, 3, 1),
        new MulTwoList(1, 2, 3),
        new ShiftOneList(2, 1, 4),
        new MulTwoList(4, 3, 1),
        new MulTwoList(0, 1, 0)},
    {
        new ShiftOneList(2, 0, 1),
        new ShiftOneList(1, 3, 2),
        new AddManyList({0, 1, 2}, 0),
        new AddTwoList(2, 2, 1),
        new MulManyList({1, 0}, 2),
        new MulTwoList(1, 2, 0)}
};

