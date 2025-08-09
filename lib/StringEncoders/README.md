# StringEncoders
This C++ library provides binary and hex string encoding function. 

The test functions are included in the library itself so they can eaily be called during device self-tests.

# Testing
The following command will build and run the unit tests for the encoders.

```bash
make clean && make all && make run-tests
```

Unit test output:
```
rm -rf ./build
mkdir -p ./build
mkdir -p ./build/tests
gcc -I ./include ./src/BinaryString.cpp ./src/BinaryStringTest.cpp -o ./build/tests/BinaryStringTest
gcc -I ./include ./src/HexString.cpp ./src/HexStringTest.cpp -o ./build/tests/HexStringTest
./build/tests/BinaryStringTest
Running test_byte_to_bits()... Passed
Running test_byte_array_to_bits()... Passed
Bits of "Hello!": 010010000110010101101100011011000110111100100001
./build/tests/HexStringTest
Running test_byte_array_to_hex()... Passed
Hex bytes of "Hello!": 48656C6C6F21
```
