#include <iostream>
#include <cstdint>
#include <cstring>
#include <cstdlib>   // for rand, srand
#include <ctime>     // for time
#include <random>
#include <chrono>
#define rounds 1
#define n 1<<20
uint32_t state[16];

// Bitwise Rotate Left Function
uint32_t bit_rl(uint32_t value, int shift) {
    return (value << shift) | (value >> (32 - shift));
}

// Quarter-Round Function
void QR(uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d) {
    a += b; d ^= a; d = bit_rl(d, 16);
    c += d; b ^= c; b = bit_rl(b, 12);
    a += b; d ^= a; d = bit_rl(d, 8);
    c += d; b ^= c; b = bit_rl(b, 7);
}

// ChaCha Block
void chacha_block(const uint32_t state[16], uint32_t output[16]) {
    std::memcpy(output, state, 16 * sizeof(uint32_t));
    for (int i = 0; i < rounds; ++i) {  
        // Odd round
        if(i % 2 == 0){
            QR(output[0], output[4], output[8], output[12]);
            QR(output[1], output[5], output[9], output[13]);
            QR(output[2], output[6], output[10], output[14]);
            QR(output[3], output[7], output[11], output[15]);
        }
        // Even round
        else{
            QR(output[0], output[5], output[10], output[15]);
            QR(output[1], output[6], output[11], output[12]);
            QR(output[2], output[7], output[8], output[13]);
            QR(output[3], output[4], output[9], output[14]);
        }
    }

    //Final Ciphertext
    for (int i = 0; i < 16; ++i) {
        output[i] += state[i];
    }
}

// Bitwise Rotate Right Function
uint32_t bit_rr(uint32_t value, int shift) {
    return (value >> shift) | (value << (32 - shift));
}

// Reverse Quarter-Round Function
void QR_inverse(uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d) {
    b = bit_rr(b, 7) ^ c; c = c - d;
    d = bit_rr(d, 8) ^ a; a = a - b;
    b = bit_rr(b, 12) ^ c; c = c - d;
    d = bit_rr(d, 16) ^ a; a = a - b;
}

int main() {
    // RNG
    std::random_device rd;
    unsigned seed = rd() ^ (unsigned)std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFFu);
    uint64_t matches = 0;

    // Coefficients for ChaCha20
    const uint32_t C0 = 0x61707865;
    const uint32_t C1 = 0x3320646e;
    const uint32_t C2 = 0x79622d32;
    const uint32_t C3 = 0x6b206574;

    for (uint64_t t = 0; t < n; ++t) {
        for (int i = 0; i < 16; ++i) state[i] = dis(gen);
        state[0] = C0;
        state[1] = C1;
        state[2] = C2;
        state[3] = C3;

    std::cout << "Randomly generated initial state:" << std::endl;
    for (int i = 0; i < 16; ++i) {
        printf("%08x ", state[i]);
        if ((i + 1) % 4 == 0)
            std::cout << std::endl;
    }
    //std::cout << "Entropy: " << rd.entropy() << std::endl;


    // Output block
    uint32_t output[16];
    chacha_block(state, output);

    std::cout << "\nChaCha20 output block:" << std::endl;
    for (int i = 0; i < 16; ++i) {
        printf("%08x ", output[i]);
        if ((i + 1) % 4 == 0)
            std::cout << std::endl;
    }

    // Reverse addition step
    for (int i = 0; i < 16; ++i) {
        output[i] -= state[i];
    }

    // Reverse rounds
    for (int i = rounds - 1; i >= 0; --i) {
        // Reverse even round
        if(i % 2 == 1){
            QR_inverse(output[3], output[4], output[9], output[14]);
            QR_inverse(output[2], output[7], output[8], output[13]);
            QR_inverse(output[1], output[6], output[11], output[12]);
            QR_inverse(output[0], output[5], output[10], output[15]);
        }
        // Reverse odd round
        else{
            QR_inverse(output[3], output[7], output[11], output[15]);
            QR_inverse(output[2], output[6], output[10], output[14]);
            QR_inverse(output[1], output[5], output[9], output[13]);
            QR_inverse(output[0], output[4], output[8], output[12]);
        }
    }

    std::cout << "\nRecovered initial state:" << std::endl;
    for (int i = 0; i < 16; ++i) {
        printf("%08x ", output[i]);
        if ((i + 1) % 4 == 0)
            std::cout << std::endl;
    }

    return 0;

}
