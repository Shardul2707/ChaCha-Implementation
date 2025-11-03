#include <iostream>
#include <cstdint>
#include <cstring>
#include <cstdlib>   // for rand, srand
#include <ctime>     // for time
#include <random>
#include <chrono>
#define rounds 3

 // Record start time
    auto start = std::chrono::high_resolution_clock::now();

// rotate left
uint32_t bit_rl(uint32_t value, int shift) {
    return (value << shift) | (value >> (32 - shift));
}

// rotate right
uint32_t bit_rr(uint32_t value, int shift) {
    return (value >> shift) | (value << (32 - shift));
}

// Quarter round
void QR(uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d) {
    a += b; d ^= a; d = bit_rl(d, 16);
    c += d; b ^= c; b = bit_rl(b, 12);
    a += b; d ^= a; d = bit_rl(d, 8);
    c += d; b ^= c; b = bit_rl(b, 7);
}

// inverse QR
void QR_inverse(uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d) {
    b = bit_rr(b, 7) ^ c; c = c - d;
    d = bit_rr(d, 8) ^ a; a = a - b;
    b = bit_rr(b, 12) ^ c; c = c - d;
    d = bit_rr(d, 16) ^ a; a = a - b;
}

// ChaCha block
void chacha_block(const uint32_t state[16], uint32_t output[16]) {
    std::memcpy(output, state, 16 * sizeof(uint32_t));
    for (int i = 0; i < rounds; ++i) {
        if (i % 2 == 0) {
            QR(output[0], output[4], output[8], output[12]);
            QR(output[1], output[5], output[9], output[13]);
            QR(output[2], output[6], output[10], output[14]);
            QR(output[3], output[7], output[11], output[15]);
        } else {
            QR(output[0], output[5], output[10], output[15]);
            QR(output[1], output[6], output[11], output[12]);
            QR(output[2], output[7], output[8], output[13]);
            QR(output[3], output[4], output[9], output[14]);
        }
    }
}

// Get k-th bit (0 or 1)
inline uint32_t get_bit(uint32_t value, int k) {
    return (value >> k) & 1U;
}

int main(int argc, char** argv) {

    // RNG
    std::random_device rd;
    unsigned seed = rd() ^ (unsigned)std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFFu);

    uint64_t n = 1048576; // Number of trials 2^20
    uint64_t matches = 0;

    // Coefficients for ChaCha20
    const uint32_t C0 = 0x61707865;
    const uint32_t C1 = 0x3320646e;
    const uint32_t C2 = 0x79622d32;
    const uint32_t C3 = 0x6b206574;

    for (uint64_t t = 0; t < n; ++t) {
        uint32_t state[16];
        for (int i = 0; i < 16; ++i) state[i] = dis(gen);
        state[0] = C0;
        state[1] = C1;
        state[2] = C2;
        state[3] = C3;

        uint32_t X[16], X_prime[16];
        // Initialize X and X'
        for (int i = 0; i < 16; ++i) X[i] = state[i];
        for (int i = 0; i < 16; ++i) X_prime[i] = X[i];
        X_prime[13] ^= (1U << 13); // flip that bit

        // Run the block function
        chacha_block(X, X);
        chacha_block(X_prime, X_prime);

        // Compare 0th bit of 11th word
        uint32_t b1 = get_bit(X[11], 0);
        uint32_t b2 = get_bit(X_prime[11], 0);
        if (b1 == b2) ++matches;
    }

    // print only the probability value (matches / n) with 6 decimal places
    double prob = static_cast<double>(matches) / static_cast<double>(n);
    std::printf("%.6f\n", prob);
    std::printf("%.6f\n", 2*prob-1);
    
    return 0;

    // Record end time
    auto end = std::chrono::high_resolution_clock::now();

    // Calculate duration in milliseconds
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Execution time: " << duration.count() << " ms" << std::endl;
}
