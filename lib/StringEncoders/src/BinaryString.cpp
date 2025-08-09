#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Mask and shift each bit and convert to the ascii value
void byte_to_bits(unsigned const char byte, char *bit_str) {
    int o = 0;

    memset(bit_str, 0x0, 9 * sizeof(char));

    for (int i = 7; i > -1; i--) {
        bit_str[o] = ((byte & (1 << i)) != 0) + '0';
        o++;
    }
}

void test_byte_to_bits() {
    char bit_str[9];

    byte_to_bits('A', bit_str);
#ifdef DEBUG

    printf("\n\ntest_byte_to_bits \"A\": %s\n", bit_str);
#endif
    assert(strcmp(bit_str, "01000001") == 0);

    byte_to_bits(0xff, bit_str);
#ifdef DEBUG
    printf("test_byte_to_bits \"0xff\": %s\n", bit_str);
#endif
    assert(strcmp(bit_str, "11111111") == 0);

    byte_to_bits(0xf0, bit_str);
#ifdef DEBUG
    printf("test_byte_to_bits \"0xf0\": %s\n", bit_str);
#endif
    assert(strcmp(bit_str, "11110000") == 0);

    byte_to_bits(0x0f, bit_str);
#ifdef DEBUG
    printf("test_byte_to_bits \"0x0f\": %s\n\n", bit_str);
#endif
    assert(strcmp(bit_str, "00001111") == 0);
}

void byte_array_to_bits(char *output, const int output_len, const char *input, const int input_len) {
    char bit_str[9];

    memset(output, 0x0, output_len * sizeof(char));

    for (int i=0; i < input_len; i++) {
        if (strlen(output) >= output_len) {
            break;
        } 
        byte_to_bits(input[i], bit_str);
        strcpy(output + (strlen(output)), bit_str);
    }
}

void test_byte_array_to_bits() {
    char input[] = "Hello!";
    int input_len = strlen(input);
    int output_len = input_len * 8;
    char output[output_len];
    byte_array_to_bits(output, output_len, input, input_len);
    assert(strcmp(output, "010010000110010101101100011011000110111100100001") == 0);
}

void print_byte_array_bits(const char *input, const unsigned int input_len) {
    const unsigned int output_len = input_len * 8;
    char output_buffer[output_len];
    memset(output_buffer, 0x00, output_len * sizeof(char));
    byte_array_to_bits(output_buffer, output_len, input, input_len);
    printf("Bits of \"%s\": %s\n", input, output_buffer);
}
