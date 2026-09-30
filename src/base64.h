#ifndef BASE64_H
#define BASE64_H

#include <string>
#include <vector>

/**
 * @brief Класс для кодирования и декодирования данных в формате Base64 (RFC 4648).
 * 
 * В соответствии с заданием, двоичные данные в VFS хранятся в Base64.
 * Декодер преобразует 4 6-битных символа Base64 обратно в 3 8-битных байта.
 */
class Base64 {
public:
    // Декодирование строки Base64 в последовательность байт (строку)
    static std::string decode(const std::string& input);

    // Кодирование строки или байт в формат Base64
    static std::string encode(const std::string& input);

private:
    static const std::string BASE64_CHARS;
    static inline bool isBase64(unsigned char c);
};

#endif // BASE64_H
