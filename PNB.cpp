#include <iostream>
#include <cstdint>
#include <cstring>
#include <cstdlib>   // for rand, srand
#include <ctime>     // for time
#include <random>
#include <chrono>

#define rounds 6
#define rounds_ 3

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
    b = bit_rr(b, 7) ^ c;  c = c - d;
    d = bit_rr(d, 8) ^ a;  a = a - b;
    b = bit_rr(b, 12) ^ c; c = c - d;
    d = bit_rr(d, 16) ^ a; a = a - b;
}

// ChaCha block
void chacha_block(const uint32_t state[16], uint32_t output[16], uint32_t S3[16]) {
    std::memcpy(output, state, 16 * sizeof(uint32_t));
    for (int i = 0; i < rounds; ++i) {
        if (i % 2 == 0) {
            QR(output[0], output[4], output[8],  output[12]);
            QR(output[1], output[5], output[9],  output[13]);
            QR(output[2], output[6], output[10], output[14]);
            QR(output[3], output[7], output[11], output[15]);
        } else {
            QR(output[0], output[5], output[10], output[15]);
            QR(output[1], output[6], output[11], output[12]);
            QR(output[2], output[7], output[8],  output[13]);
            QR(output[3], output[4], output[9],  output[14]);
        }
        if (i == 2) {
            for (int j = 0; j < 16; ++j) {
                S3[j] = output[j];
            }
        }
    }
}

// Get k-th bit (0 or 1)
inline uint32_t get_bit(uint32_t value, int k) {
    return (value >> k) & 1U;
}

int main() {
    int count_ = 0;
    std::random_device rd;
    unsigned seed = rd() ^ (unsigned)std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFFu);

    uint64_t n = 10000; // Number of trials

    // ChaCha20 constants
    const uint32_t C0 = 0x61707865;
    const uint32_t C1 = 0x3320646e;
    const uint32_t C2 = 0x79622d32;
    const uint32_t C3 = 0x6b206574;

    // Pre-generate random states
    uint32_t states[n][16];
    for (uint64_t t = 0; t < n; ++t) {
        for (int i = 0; i < 16; ++i) states[t][i] = dis(gen);
        states[t][0] = C0;
        states[t][1] = C1;
        states[t][2] = C2;
        states[t][3] = C3;
    }

    // Loop over all 256 (f, I) combinations
    for (int f = 4; f < 12; f++) {
        for (int I = 0; I < 32; I++) {
            int ctr = 0;

            for (uint64_t t = 0; t < n; ++t) {
                uint32_t X[16], X_prime[16], X_[16], X3[16], X_3[16], Ini[16];
                uint32_t Z[16], Z_[16], Xt[16], Xt_prime[16], XT[16], XT_[16];

                // Initialize states
                for (int i = 0; i < 16; ++i) {
                    Ini[i] = states[t][i];
                    X[i] = states[t][i];
                    X_prime[i] = X[i];
                }
                X_prime[13] ^= (1U << 13); // flip bit
                for (int i = 0; i < 16; ++i) X_[i] = X_prime[i];

                // Run block function
                chacha_block(Ini, X, X3);
                chacha_block(X_prime, X_, X_3);

                for (int i = 0; i < 16; i++) {
                    Z[i]  = X[i] + Ini[i];
                    Z_[i] = X_prime[i] + X_[i];
                }

                for (int k = 0; k < 16; k++) {
                    Xt[k]       = Ini[k];
                    Xt_prime[k] = Xt[k];
                }
                Xt_prime[13] ^= (1U << 13);
                Xt[f] ^= (1U << I);
                Xt_prime[f] ^= (1U << I);

                for (int i = 0; i < 16; i++) {
                    XT[i]  = Z[i]  - Xt[i];
                    XT_[i] = Z_[i] - Xt_prime[i];
                }

                // Inverse 3 rounds for XT
                for (int i = rounds-1; i > rounds_ - 1; --i) {
                    if (i % 2 == 1) {
                        QR_inverse(XT[3], XT[4], XT[9],  XT[14]);
                        QR_inverse(XT[2], XT[7], XT[8],  XT[13]);
                        QR_inverse(XT[1], XT[6], XT[11], XT[12]);
                        QR_inverse(XT[0], XT[5], XT[10], XT[15]);
                    } else {
                        QR_inverse(XT[3], XT[7], XT[11], XT[15]);
                        QR_inverse(XT[2], XT[6], XT[10], XT[14]);
                        QR_inverse(XT[1], XT[5], XT[9],  XT[13]);
                        QR_inverse(XT[0], XT[4], XT[8],  XT[12]);
                    }
                }

                // Inverse 3 rounds for XT_
                for (int i = rounds-1; i > rounds_ - 1; --i) {
                    if (i % 2 == 1) {
                        QR_inverse(XT_[3], XT_[4], XT_[9],  XT_[14]);
                        QR_inverse(XT_[2], XT_[7], XT_[8],  XT_[13]);
                        QR_inverse(XT_[1], XT_[6], XT_[11], XT_[12]);
                        QR_inverse(XT_[0], XT_[5], XT_[10], XT_[15]);
                    } else {
                        QR_inverse(XT_[3], XT_[7], XT_[11], XT_[15]);
                        QR_inverse(XT_[2], XT_[6], XT_[10], XT_[14]);
                        QR_inverse(XT_[1], XT_[5], XT_[9],  XT_[13]);
                        QR_inverse(XT_[0], XT_[4], XT_[8],  XT_[12]);
                    }
                }

                // Compare 0th bit of 11th word
                uint32_t b1 = get_bit(X3[11], 0);
                uint32_t b2 = get_bit(X_3[11], 0);
                uint32_t b3 = get_bit(XT[11], 0);
                uint32_t b4 = get_bit(XT_[11], 0);

                if ((b1 ^ b2) == (b3 ^ b4)) {
                    ++ctr;
                }
                
            }

            // print probability for this (f, I)
            double prob = static_cast<double>(ctr) / static_cast<double>(n);
            double gamma_i = (2.0 * ctr / static_cast<double>(n)) - 1.0;
            if(gamma_i >= 0.6 || gamma_i <= -0.6) {
                std::printf("f=%2d, I=%2d, prob=%.6f\n", f, I, prob);
                std::printf("f=%2d, I=%2d, Bit=%2d, gamma=%.6f\n", f, I, (f-4)*32 + I, gamma_i);
                ++count_;
            }
        }
    }

    std::cout << "Total probabilities printed: " << count_ << std::endl;
    return 0;
}