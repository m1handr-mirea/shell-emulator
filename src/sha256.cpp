#include "sha256.h"
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cstring>

// Константы первых 32 бит дробных частей кубических корней первых 64 простых чисел
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Циклический сдвиг вправо
uint32_t Sha256::rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

// Вспомогательные битовые функции SHA-256
uint32_t Sha256::choose(uint32_t e, uint32_t f, uint32_t g) {
    return (e & f) ^ (~e & g);
}

uint32_t Sha256::majority(uint32_t a, uint32_t b, uint32_t c) {
    return (a & b) ^ (a & c) ^ (b & c);
}

uint32_t Sha256::sig0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

uint32_t Sha256::sig1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

// Инициализация начальных хэш-значений H0..H7 (первые 32 бита дробных частей квадратных корней первых 8 простых чисел)
Sha256::Sha256() : bitLength_(0), bufferLength_(0) {
    state_[0] = 0x6a09e667;
    state_[1] = 0xbb67ae85;
    state_[2] = 0x3c6ef372;
    state_[3] = 0xa54ff53a;
    state_[4] = 0x510e527f;
    state_[5] = 0x9b05688c;
    state_[6] = 0x1f83d9ab;
    state_[7] = 0x5be0cd19;
}

// Преобразование одного 512-битного блока
void Sha256::transform(const uint8_t block[64]) {
    uint32_t w[64];

    // Первые 16 32-битных слов из исходного блока (big-endian)
    for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<uint32_t>(block[i * 4]) << 24) |
               (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
               (static_cast<uint32_t>(block[i * 4 + 2]) << 8) |
               (static_cast<uint32_t>(block[i * 4 + 3]));
    }

    // Расширение до 64 слов
    for (int i = 16; i < 64; ++i) {
        uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    uint32_t a = state_[0];
    uint32_t b = state_[1];
    uint32_t c = state_[2];
    uint32_t d = state_[3];
    uint32_t e = state_[4];
    uint32_t f = state_[5];
    uint32_t g = state_[6];
    uint32_t h = state_[7];

    // 64 раунда сжатия
    for (int i = 0; i < 64; ++i) {
        uint32_t t1 = h + sig1(e) + choose(e, f, g) + K[i] + w[i];
        uint32_t t2 = sig0(a) + majority(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
}

void Sha256::update(const uint8_t* data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        buffer_[bufferLength_++] = data[i];
        bitLength_ += 8;
        if (bufferLength_ == 64) {
            transform(buffer_);
            bufferLength_ = 0;
        }
    }
}

void Sha256::update(const std::string& data) {
    update(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

std::string Sha256::finalize() {
    // Дополнение (padding): бит '1' (байт 0x80)
    buffer_[bufferLength_++] = 0x80;

    // Если не хватает места для 64-битного счетчика длины (8 байт), дополняем нулями и трансформируем
    if (bufferLength_ > 56) {
        while (bufferLength_ < 64) {
            buffer_[bufferLength_++] = 0x00;
        }
        transform(buffer_);
        bufferLength_ = 0;
    }

    while (bufferLength_ < 56) {
        buffer_[bufferLength_++] = 0x00;
    }

    // Запись исходной длины сообщения в битах (big-endian 64 бита)
    for (int i = 7; i >= 0; --i) {
        buffer_[bufferLength_++] = static_cast<uint8_t>((bitLength_ >> (i * 8)) & 0xff);
    }
    transform(buffer_);

    // Форматирование результата в шестнадцатеричную строку (64 символа)
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 8; ++i) {
        oss << std::setw(8) << state_[i];
    }
    return oss.str();
}

std::string Sha256::hashString(const std::string& input) {
    Sha256 ctx;
    ctx.update(input);
    return ctx.finalize();
}

std::string Sha256::hashFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    Sha256 ctx;
    char buffer[4096];
    while (file.read(buffer, sizeof(buffer))) {
        ctx.update(reinterpret_cast<const uint8_t*>(buffer), sizeof(buffer));
    }
    if (file.gcount() > 0) {
        ctx.update(reinterpret_cast<const uint8_t*>(buffer), static_cast<size_t>(file.gcount()));
    }
    return ctx.finalize();
}
