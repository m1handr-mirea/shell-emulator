#ifndef JSON_H
#define JSON_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>

/**
 * @brief Простой парсер JSON для представления структуры VFS.
 * 
 * Написан в учебных целях на чистом C++17 без внешних библиотек.
 * Поддерживает базовые типы JSON:
 * - Объект (map<string, JsonValue>)
 * - Массив (vector<JsonValue>)
 * - Строка (string)
 * - Число (double)
 * - Булево значение (bool)
 * - Null
 */
class JsonValue {
public:
    enum class Type {
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object
    };

    JsonValue();
    explicit JsonValue(bool b);
    explicit JsonValue(double d);
    explicit JsonValue(const std::string& s);
    explicit JsonValue(const char* s);
    explicit JsonValue(const std::vector<JsonValue>& arr);
    explicit JsonValue(const std::map<std::string, JsonValue>& obj);

    Type getType() const { return type_; }
    bool isNull() const { return type_ == Type::Null; }
    bool isBool() const { return type_ == Type::Boolean; }
    bool isNumber() const { return type_ == Type::Number; }
    bool isString() const { return type_ == Type::String; }
    bool isArray() const { return type_ == Type::Array; }
    bool isObject() const { return type_ == Type::Object; }

    bool asBool() const;
    double asNumber() const;
    const std::string& asString() const;
    const std::vector<JsonValue>& asArray() const;
    const std::map<std::string, JsonValue>& asObject() const;

    bool contains(const std::string& key) const;
    const JsonValue& operator[](const std::string& key) const;
    const JsonValue& operator[](size_t index) const;

    // Парсинг JSON из строки
    static JsonValue parse(const std::string& jsonText);

private:
    Type type_;
    bool boolVal_;
    double numVal_;
    std::string strVal_;
    std::vector<JsonValue> arrVal_;
    std::map<std::string, JsonValue> objVal_;

    // Внутренний класс лексера и парсера
    class Parser {
    public:
        explicit Parser(const std::string& src);
        JsonValue parseValue();

    private:
        const std::string& src_;
        size_t pos_;

        void skipWhitespace();
        char peek();
        char get();
        std::string parseString();
        double parseNumber();
        JsonValue parseObject();
        JsonValue parseArray();
        void expect(char c);
    };
};

#endif // JSON_H
