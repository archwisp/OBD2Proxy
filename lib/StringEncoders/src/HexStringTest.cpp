#include <stdio.h>
#include "HexString.h"

#define DEBUG

int main(int argc, char *argv[]) {
    printf("Running test_byte_array_to_hex()... ");
    test_byte_array_to_hex();
    printf("Passed\n");
#ifdef DEBUG 
    print_byte_array_hex("Hello!", 6);
#endif
}
