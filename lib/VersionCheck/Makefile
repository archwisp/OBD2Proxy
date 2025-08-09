CC=gcc
CPPFLAGS=
SRC_DIR=./src
BUILD_DIR=./build
SO_DIR=$(BUILD_DIR)/lib
INCLUDE_DIR=$(BUILD_DIR)/include
TEST_DIR=$(BUILD_DIR)/tests
MKDIR = mkdir -p

.PHONY: directories all

build: directories VersionCheckShared

all: directories build tests 

directories: ${SO_DIR} ${INCLUDE_DIR} ${TEST_DIR}

tests: VersionCheckTest

${SO_DIR}:
	${MKDIR} ${SO_DIR}

${INCLUDE_DIR}:
	${MKDIR} ${INCLUDE_DIR}

${TEST_DIR}:
	${MKDIR} ${TEST_DIR}

VersionCheckShared: ${SRC_DIR}/VersionCheck.cpp
	$(shell cp ./include/VersionCheck.h $(INCLUDE_DIR)/VersionCheck.h)
	$(CC) $(CPPFLAGS) -I $(INCLUDE_DIR) -shared -fPIC ${SRC_DIR}/VersionCheck.cpp -o ${SO_DIR}/libVersionCheck.so

VersionCheckTest: ${SRC_DIR}/VersionCheckTest.cpp
	$(CC) $(CPPFLAGS) -I $(INCLUDE_DIR) -L$(SO_DIR) ${SRC_DIR}/VersionCheckTest.cpp -o ${TEST_DIR}/VersionCheckTest -lVersionCheck

clean:
	rm -rf ./build

run-tests:
	LD_LIBRARY_PATH=$(SO_DIR) ${TEST_DIR}/VersionCheckTest
