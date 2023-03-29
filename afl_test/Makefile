CXX=clang++-14
AFLCC=afl-cc -fprofile-arcs -ftest-coverage
PB_SRC=../src/proto/fhe.pb.cc
PROTOBUF_DIR=/usr/local/include/google/protobuf
PROTOBUF_LIB=/usr/local/lib/libprotobuf.so

INC=-I$(PROTOBUF_DIR)/include

all: vuln

$(TARGET).so: $(TARGET).cc $(PB_SRC)
	$(CXX) -fPIC -c $^ $(INC)
	$(CXX) -shared -Wall -O3 -o $@ *.o $(LPM_LIB) $(PROTOBUF_LIB)

vuln: harness.cpp $(PB_SRC) 
	$(AFLCC) -o $@ $^ -lstdc++ $(INC) $(PROTOBUF_LIB)
.PHONY: clean
clean: 
	rm *.gcno *.gcda vuln