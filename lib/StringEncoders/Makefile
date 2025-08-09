CC=gcc
CPPFLAGS=
SRC_DIR=./src
BUILD_DIR=./build
SO_DIR=$(BUILD_DIR)/lib
INCLUDE_DIR=$(BUILD_DIR)/include
TEST_DIR=$(BUILD_DIR)/tests
MKDIR = mkdir -p

.PHONY: directories all

build: directories BinaryStringShared HexStringShared

all: directories build tests 

directories: ${SO_DIR} ${INCLUDE_DIR} ${TEST_DIR}

tests: BinaryStringTest HexStringTest

${SO_DIR}:
	${MKDIR} ${SO_DIR}

${INCLUDE_DIR}:
	${MKDIR} ${INCLUDE_DIR}

${TEST_DIR}:
	${MKDIR} ${TEST_DIR}

BinaryStringShared: ${SRC_DIR}/BinaryString.cpp
	$(CC) $(CPPFLAGS) -shared -fPIC ${SRC_DIR}/BinaryString.cpp -o ${SO_DIR}/libBinaryString.so
	$(shell cp ./include/BinaryString.h $(INCLUDE_DIR)/BinaryString.h)

HexStringShared: ${SRC_DIR}/HexString.cpp
	$(CC) $(CPPFLAGS) -shared -fPIC ${SRC_DIR}/HexString.cpp -o ${SO_DIR}/libHexString.so
	$(shell cp ./include/HexString.h $(INCLUDE_DIR)/HexString.h)

BinaryStringTest: ${SRC_DIR}/BinaryStringTest.cpp
	$(CC) $(CPPFLAGS) -I $(INCLUDE_DIR) -L$(SO_DIR) ${SRC_DIR}/BinaryStringTest.cpp -o ${TEST_DIR}/BinaryStringTest -lBinaryString

HexStringTest: ${SRC_DIR}/HexStringTest.cpp
	$(CC) $(CPPFLAGS) -I $(INCLUDE_DIR) -L$(SO_DIR) ${SRC_DIR}/HexString.cpp ${SRC_DIR}/HexStringTest.cpp -o ${TEST_DIR}/HexStringTest -lHexString

clean:
	rm -rf ./build

run-tests:
	LD_LIBRARY_PATH=$(SO_DIR) ${TEST_DIR}/BinaryStringTest
	LD_LIBRARY_PATH=$(SO_DIR) ${TEST_DIR}/HexStringTest
