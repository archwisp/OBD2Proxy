#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void byte_array_to_hex(char *output, const unsigned int output_len, const char *input, const unsigned int input_len) {
    for (unsigned int i = 0; i < input_len; i++) {
        snprintf(output + (i * 2), output_len, "%02X", input[i]);
    }
}

void test_byte_array_to_hex() {
    char output[16];
    memset(output, 0x0, 16); 
    byte_array_to_hex(output, 10, "Hello", 5);
    assert(strcmp(output, "48656C6C6F") == 0);

    // Test with dynamic memory allocation

    int *m = (int *)calloc(16, sizeof(char));

    if (m == nullptr) {
        printf("test_byte_array_to_hex memory allocation failed. Exiting.");
        return;
    }

    memset(m, 0x0, 16 * sizeof(char));
    byte_array_to_hex((char*)m, 12, "Hello\0", 6);
    assert(strcmp(output, "48656C6C6F") == 0);
    free(m);


    char byte_array[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
    byte_array_to_hex(output, 12, byte_array, 6);
    assert(strcmp(output, "001122334455") == 0);
}

void print_byte_array_hex(const char *input, const unsigned int input_len) {
    const unsigned int output_len = input_len * 2;
    char output_buffer[output_len];
    memset(output_buffer, 0x0, output_len * sizeof(char));
    byte_array_to_hex(output_buffer, output_len, input, input_len);
    printf("Hex bytes of \"%s\": %s\n", input, output_buffer);
}

