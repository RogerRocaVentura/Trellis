#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#include <float.h>

typedef struct {
    int state_id;
    _Complex float qam_symbol;
    int next_state[4]; 
    char bits[2];
} TrellisState;

typedef struct {
    TrellisState* states;
    int num_states;
    int current_state;
} Trellis;

#define QAM4_LEVEL 1.0


void initializeTrellis(Trellis* trellis) {
    trellis->num_states = 4;
    trellis->states = (TrellisState*)malloc(sizeof(TrellisState) * 4);

    if (trellis->states == NULL) {
        printf("Error allocating memory for trellis states.\n");
        exit(1);
    }

    _Complex float qam4_constellation[4] = {
        -QAM4_LEVEL - QAM4_LEVEL * I, // 00
        -QAM4_LEVEL + QAM4_LEVEL * I, // 01
        QAM4_LEVEL - QAM4_LEVEL * I,  // 10
        QAM4_LEVEL + QAM4_LEVEL * I   // 11
    };

    for (int i = 0; i < 4; ++i) {
        trellis->states[i].state_id = i;
        trellis->states[i].qam_symbol = qam4_constellation[i];
        trellis->states[i].bits[0] = (i & 2) ? '1' : '0'; 
        trellis->states[i].bits[1] = (i & 1) ? '1' : '0'; 
    }
}


int mod_4QAM(char *bits, int numbits, _Complex float *symbols) {
    if (numbits != 2) {
        printf("Warning: mod_4QAM received an unexpected number of bits (%d)\n", numbits);
        return 0;
    }

    float real_part = (bits[0] == '1' ? -1 : 1) * QAM4_LEVEL;
    float imag_part = (bits[1] == '1' ? -1 : 1) * QAM4_LEVEL;
    *symbols = real_part + imag_part * I;

    printf("mod_4QAM: Bits: %s, Symbol: %f + %fi\n", bits, creal(*symbols), cimag(*symbols));

    return 1;
}

void encodeData(Trellis* trellis, const char* input_data, _Complex float* encoded_symbols, int num_symbols) {
    int len = strlen(input_data);
    trellis->current_state = 0;

    for (int i = 0; i < len; i += 2) {
        char bits[3] = {0};
        strncpy(bits, input_data + i, 2);
        bits[2] = '\0';

        mod_4QAM(bits, 2, &encoded_symbols[i / 2]);

        printf("Encoding bits: %s -> Symbol: %f + %fi, Next state: %d\n", 
               bits, creal(encoded_symbols[i / 2]), cimag(encoded_symbols[i / 2]), 
               trellis->current_state);

        trellis->current_state = (trellis->current_state + 1) % trellis->num_states;
    }
}

void decodeData(Trellis* trellis, const _Complex float* encoded_symbols, char* decoded_data, int num_symbols) {
    printf("Decoding %d symbols.\n", num_symbols);

    for (int i = 0; i < num_symbols; ++i) {
        int closest_state = 0;
        float min_distance = FLT_MAX;

        for (int j = 0; j < trellis->num_states; ++j) {
            float distance = cabs(trellis->states[j].qam_symbol - encoded_symbols[i]);
            printf("State %d, Symbol: %f+%fi, Distance: %f\n", j, 
                   creal(trellis->states[j].qam_symbol), cimag(trellis->states[j].qam_symbol), distance);
            if (distance < min_distance) {
                min_distance = distance;
                closest_state = j;
            }
        }

        decoded_data[i * 2] = trellis->states[closest_state].bits[0] == '0' ? '1' : '0'; 
        decoded_data[i * 2 + 1] = trellis->states[closest_state].bits[1] == '0' ? '1' : '0'; 

        printf("Decoded Symbol: %f + %fi, Closest State: %d, Bits: %c%c\n", 
               creal(encoded_symbols[i]), cimag(encoded_symbols[i]), 
               closest_state, decoded_data[i * 2], decoded_data[i * 2 + 1]);
    }
    decoded_data[num_symbols * 2] = '\0';
}

int main() {
    Trellis trellis;
    initializeTrellis(&trellis);

    const char* input_data = "011011001010"; // Posem un input controlat i després el comparem amb els decodejats.
    int input_length = strlen(input_data);
    int num_encoded_symbols = (input_length + 1) / 2;

    _Complex float encoded_symbols[num_encoded_symbols];
    char decoded_data[input_length + 1];
    memset(decoded_data, 0, sizeof(decoded_data));

    encodeData(&trellis, input_data, encoded_symbols, num_encoded_symbols);
    decodeData(&trellis, encoded_symbols, decoded_data, num_encoded_symbols);

    printf("Original Data: %s\n", input_data); // Els bits que hem introduït
    printf("Decoded Data: %s\n", decoded_data); // Els bits decodificats 

    free(trellis.states);
    return 0;
}
