#include <string>
#include <stdlib.h>
#include <fstream>
#include "../proto/proto_setting.h"

using namespace std;

class openfhe_error : public std::runtime_error {
    std::string filename;
    int linenum;
    std::string message;

public:
    openfhe_error(const std::string& file, int line, const std::string& what)
        : std::runtime_error(what), filename(file), linenum(line) {
        message = filename + ":" + std::to_string(linenum) + " " + what;
    }

    const char* what() const throw() {
        return message.c_str();
    }

    const std::string& GetFilename() const {
        return filename;
    }
    int GetLinenum() const {
        return linenum;
    }
};

class config_error : public openfhe_error {
public:
    config_error(const std::string& file, int line, const std::string& what) : openfhe_error(file, line, what) {}
};

class math_error : public openfhe_error {
public:
    math_error(const std::string& file, int line, const std::string& what) : openfhe_error(file, line, what) {}
};

class not_implemented_error : public openfhe_error {
public:
    not_implemented_error(const std::string& file, int line, const std::string& what)
        : openfhe_error(file, line, what) {}
};

class not_available_error : public openfhe_error {
public:
    not_available_error(const std::string& file, int line, const std::string& what) : openfhe_error(file, line, what) {}
};

class type_error : public openfhe_error {
public:
    type_error(const std::string& file, int line, const std::string& what) : openfhe_error(file, line, what) {}
};

// use this error when serializing openfhe objects
class serialize_error : public openfhe_error {
public:
    serialize_error(const std::string& file, int line, const std::string& what) : openfhe_error(file, line, what) {}
};

// use this error when deserializing openfhe objects
class deserialize_error : public openfhe_error {
public:
    deserialize_error(const std::string& file, int line, const std::string& what) : openfhe_error(file, line, what) {}
};

#define OPENFHE_THROW(exc, expr) throw exc(__FILE__, __LINE__, (expr))
#define __output(...) \
    printf(__VA_ARGS__);
 
#define __format(__fmt__) "%s(%d)-<%s>: " __fmt__ "\n"

/**
 * @brief Print the file name, line number and function where the error occurred.
 **/
#define TRACE_FILE_LINE_FUNCTION(__fmt__, ...) \
    __output(__format(__fmt__), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);
#define THROW_EXCEPTION(throw_message)\
    do{\
        TRACE_FILE_LINE_FUNCTION("fhe throw exception.");\
        perror(throw_message);\
        abort();\
        exit(0);\
    }while(0)
int main(int argc, char *argv[]) {
    char* str = new char[100];
    Root input;
    // ifstream in(argv[1]);
    // input.ParseFromIstream(&in);
    // int f = 0;
    // for(int i = 0;i < 100;i++){
    //     if(i != input.param().multiplicativedepth() % 100){
    //         f = 1;
    //         break;
    //     }else delete str;
    // }
    // if(f) str[0] = '1';
    // else str[0] = '0';
    try
    {
        OPENFHE_THROW(config_error, "test");
    }
    catch(const openfhe_error& e)
    {
        THROW_EXCEPTION(e.what());
    }
    
    return 0;
}