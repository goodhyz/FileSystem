/**
 * @file encrypt.h
 * 实现SHA256加密
 */

#pragma once

#include <array>
#include <bitset>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>

class SHA256 {
public:
    SHA256() { reset(); }

    void update(const std::string &data) {
        for (char c : data) {
            update(c);
        }
    }

    std::string final() {
        std::string hash = to_hex_string(finalize());
        reset();
        return hash;
    }

private:
    static constexpr std::array<uint32_t, 64> k = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    std::array<uint32_t, 8> h;
    std::vector<uint8_t> buffer;
    uint64_t bit_length;

    void reset() {
        h = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
        buffer.clear();
        bit_length = 0;
    }

    void update(uint8_t byte) {
        buffer.push_back(byte);
        bit_length += 8;
        if (buffer.size() == 64) {
            process_block();
            buffer.clear();
        }
    }

    std::array<uint32_t, 8> finalize() {
        buffer.push_back(0x80);
        while (buffer.size() != 56) {
            if (buffer.size() == 64) {
                process_block();
                buffer.clear();
            }
            buffer.push_back(0x00);
        }

        for (int i = 7; i >= 0; --i) {
            buffer.push_back(static_cast<uint8_t>((bit_length >> (i * 8)) & 0xff));
        }

        process_block();

        return h;
    }

    void process_block() {
        std::array<uint32_t, 64> w;
        for (size_t i = 0; i < 16; ++i) {
            w[i] = (buffer[i * 4] << 24) | (buffer[i * 4 + 1] << 16) | (buffer[i * 4 + 2] << 8) | buffer[i * 4 + 3];
        }

        for (size_t i = 16; i < 64; ++i) {
            uint32_t s0 = right_rotate(w[i - 15], 7) ^ right_rotate(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = right_rotate(w[i - 2], 17) ^ right_rotate(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        std::array<uint32_t, 8> a = h;

        for (size_t i = 0; i < 64; ++i) {
            uint32_t s1 = right_rotate(a[4], 6) ^ right_rotate(a[4], 11) ^ right_rotate(a[4], 25);
            uint32_t ch = (a[4] & a[5]) ^ (~a[4] & a[6]);
            uint32_t temp1 = a[7] + s1 + ch + k[i] + w[i];
            uint32_t s0 = right_rotate(a[0], 2) ^ right_rotate(a[0], 13) ^ right_rotate(a[0], 22);
            uint32_t maj = (a[0] & a[1]) ^ (a[0] & a[2]) ^ (a[1] & a[2]);
            uint32_t temp2 = s0 + maj;

            a[7] = a[6];
            a[6] = a[5];
            a[5] = a[4];
            a[4] = a[3] + temp1;
            a[3] = a[2];
            a[2] = a[1];
            a[1] = a[0];
            a[0] = temp1 + temp2;
        }

        for (size_t i = 0; i < 8; ++i) {
            h[i] += a[i];
        }
    }

    static uint32_t right_rotate(uint32_t value, uint32_t count) {
        return (value >> count) | (value << (32 - count));
    }

    static std::string to_hex_string(const std::array<uint32_t, 8> &hash) {
        std::stringstream ss;
        for (uint32_t value : hash) {
            ss << std::hex << std::setw(8) << std::setfill('0') << value;
        }
        return ss.str();
    }
};
