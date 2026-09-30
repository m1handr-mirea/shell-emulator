#ifndef SHA256_H
#define SHA256_H

#include <string>
#include <vector>
#include <cstdint>

/**
 * @brief Класс для вычисления криптографического хеша SHA-256 (FIPS 180-4).
 * 
 * Реализация написана в учебных целях без внешних зависимостей (OpenSSL и др.),
 * что позволяет студенту легко объяснить принципы работы алгоритма хеширования:
 * - Дополнение сообщения (padding) до кратности 512 бит
 * - Инициализация 8 32-битных переменных начальными значениями
 * - 64 раунда сжатия с использованием логических функций Ch, Maj, Sigma0, Sigma1
 * - Формирование 256-битного (64 hex-символа) дайджеста
 */
class Sha256 {
public:
    Sha256();

    // Обновление состояния новыми данными
    void update(const uint8_t* data, size_t length);
    void update(const std::string& data);

    // Завершение вычисления и получение строки в hex-формате (64 символа)
    std::string finalize();

    // Статический вспомогательный метод для быстрого хеширования строки или файла
    static std::string hashString(const std::string& input);
    static std::string hashFile(const std::string& filePath);

private:
    uint32_t state_[8];        // 8 переменных состояния (A, B, C, D, E, F, G, H)
    uint64_t bitLength_;       // Общая длина сообщения в битах
    uint8_t buffer_[64];       // Буфер на один блок (512 бит = 64 байта)
    size_t bufferLength_;      // Текущее заполнение буфера

    void transform(const uint8_t block[64]);
    static uint32_t rotr(uint32_t x, uint32_t n);
    static uint32_t choose(uint32_t e, uint32_t f, uint32_t g);
    static uint32_t majority(uint32_t a, uint32_t b, uint32_t c);
    static uint32_t sig0(uint32_t x);
    static uint32_t sig1(uint32_t x);
};

#endif // SHA256_H
