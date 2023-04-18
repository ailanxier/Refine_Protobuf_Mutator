AFLCC=afl-cc -fprofile-arcs -ftest-coverage
PB_SRC=../proto/openfhe_ckks.pb.cc
PROTOBUF_DIR=/usr/local/include/google/protobuf
PROTOBUF_LIB=/usr/local/lib/libprotobuf.so

INC=-I$(PROTOBUF_DIR)/include

all: vuln

vuln: harness.cpp $(PB_SRC) 
	$(AFLCC) -o $@ $^ -lstdc++  $(INC) $(PROTOBUF_LIB)
.PHONY: clean
clean: 
	rm *.gcno *.gcda vuln