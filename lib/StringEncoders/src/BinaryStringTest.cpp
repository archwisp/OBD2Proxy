#include <stdio.h>
#include "BinaryString.h"

int main(int argc, char *argv[]) {
    printf("Running test_byte_to_bits()... ");
    test_byte_to_bits();
    printf("Passed\n");
    printf("Running test_byte_array_to_bits()... ");
    test_byte_array_to_bits();
    printf("Passed\n");
    print_byte_array_bits("Hello!", 6);
}
