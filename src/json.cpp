#include "json.h"
#include <cctype>
#include <sstream>

static const JsonValue NULL_VALUE;

JsonValue::JsonValue() : type_(Type::Null), boolVal_(false), numVal_(0.0) {}
JsonValue::JsonValue(bool b) : type_(Type::Boolean), boolVal_(b), numVal_(0.0) {}
JsonValue::JsonValue(double d) : type_(Type::Number), boolVal_(false), numVal_(d) {}
JsonValue::JsonValue(const std::string& s) : type_(Type::String), boolVal_(false), numVal_(0.0), strVal_(s) {}
JsonValue::JsonValue(const char* s) : type_(Type::String), boolVal_(false), numVal_(0.0), strVal_(s ? s : "") {}
JsonValue::JsonValue(const std::vector<JsonValue>& arr) : type_(Type::Array), boolVal_(false), numVal_(0.0), arrVal_(arr) {}
JsonValue::JsonValue(const std::map<std::string, JsonValue>& obj) : type_(Type::Object), boolVal_(false), numVal_(0.0), objVal_(obj) {}

bool JsonValue::asBool() const {
    if (!isBool()) throw std::runtime_error("JsonValue is not a boolean");
    return boolVal_;
}

double JsonValue::asNumber() const {
    if (!isNumber()) throw std::runtime_error("JsonValue is not a number");
    return numVal_;
}

const std::string& JsonValue::asString() const {
    if (!isString()) throw std::runtime_error("JsonValue is not a string");
    return strVal_;
}

const std::vector<JsonValue>& JsonValue::asArray() const {
    if (!isArray()) throw std::runtime_error("JsonValue is not an array");
    return arrVal_;
}

const std::map<std::string, JsonValue>& JsonValue::asObject() const {
    if (!isObject()) throw std::runtime_error("JsonValue is not an object");
    return objVal_;
}

bool JsonValue::contains(const std::string& key) const {
    if (!isObject()) return false;
    return objVal_.find(key) != objVal_.end();
}

const JsonValue& JsonValue::operator[](const std::string& key) const {
    if (!isObject()) return NULL_VALUE;
    auto it = objVal_.find(key);
    if (it != objVal_.end()) return it->second;
    return NULL_VALUE;
}

const JsonValue& JsonValue::operator[](size_t index) const {
    if (!isArray() || index >= arrVal_.size()) return NULL_VALUE;
    return arrVal_[index];
}

JsonValue::Parser::Parser(const std::string& src) : src_(src), pos_(0) {}

void JsonValue::Parser::skipWhitespace() {
    while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) {
        pos_++;
    }
}

char JsonValue::Parser::peek() {
    skipWhitespace();
    if (pos_ >= src_.size()) return '\0';
    return src_[pos_];
}

char JsonValue::Parser::get() {
    skipWhitespace();
    if (pos_ >= src_.size()) return '\0';
    return src_[pos_++];
}

void JsonValue::Parser::expect(char c) {
    char actual = get();
    if (actual != c) {
        throw std::runtime_error(std::string("Expected '") + c + "', got '" + actual + "'");
    }
}

std::string JsonValue::Parser::parseString() {
    expect('"');
    std::string result;
    while (pos_ < src_.size()) {
        char c = src_[pos_++];
        if (c == '"') {
            return result;
        }
        if (c == '\\') {
            if (pos_ >= src_.size()) throw std::runtime_error("Unexpected end in escape sequence");
            char esc = src_[pos_++];
            switch (esc) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                default: result += esc; break;
            }
        } else {
            result += c;
        }
    }
    throw std::runtime_error("Unterminated string in JSON");
}

double JsonValue::Parser::parseNumber() {
    size_t start = pos_;
    if (src_[pos_] == '-') pos_++;
    while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) pos_++;
    if (pos_ < src_.size() && src_[pos_] == '.') {
        pos_++;
        while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) pos_++;
    }
    std::string numStr = src_.substr(start, pos_ - start);
    return std::stod(numStr);
}

JsonValue JsonValue::Parser::parseObject() {
    expect('{');
    std::map<std::string, JsonValue> obj;
    if (peek() == '}') {
        get();
        return JsonValue(obj);
    }
    while (true) {
        skipWhitespace();
        if (peek() != '"') throw std::runtime_error("Expected string key in object");
        std::string key = parseString();
        skipWhitespace();
        expect(':');
        JsonValue val = parseValue();
        obj[key] = val;
        char next = peek();
        if (next == '}') {
            get();
            break;
        } else if (next == ',') {
            get();
        } else {
            throw std::runtime_error("Expected ',' or '}' in object");
        }
    }
    return JsonValue(obj);
}

JsonValue JsonValue::Parser::parseArray() {
    expect('[');
    std::vector<JsonValue> arr;
    if (peek() == ']') {
        get();
        return JsonValue(arr);
    }
    while (true) {
        arr.push_back(parseValue());
        char next = peek();
        if (next == ']') {
            get();
            break;
        } else if (next == ',') {
            get();
        } else {
            throw std::runtime_error("Expected ',' or ']' in array");
        }
    }
    return JsonValue(arr);
}

JsonValue JsonValue::Parser::parseValue() {
    skipWhitespace();
    char c = peek();
    if (c == '{') return parseObject();
    if (c == '[') return parseArray();
    if (c == '"') return JsonValue(parseString());
    if (std::isdigit(static_cast<unsigned char>(c)) || c == '-') return JsonValue(parseNumber());
    if (c == 't' || c == 'f') {
        std::string s;
        while (pos_ < src_.size() && std::isalpha(static_cast<unsigned char>(src_[pos_]))) {
            s += src_[pos_++];
        }
        if (s == "true") return JsonValue(true);
        if (s == "false") return JsonValue(false);
        throw std::runtime_error("Invalid boolean literal: " + s);
    }
    if (c == 'n') {
        std::string s;
        while (pos_ < src_.size() && std::isalpha(static_cast<unsigned char>(src_[pos_]))) {
            s += src_[pos_++];
        }
        if (s == "null") return JsonValue();
        throw std::runtime_error("Invalid null literal: " + s);
    }
    throw std::runtime_error(std::string("Unexpected character in JSON: '") + c + "'");
}

JsonValue JsonValue::parse(const std::string& jsonText) {
    Parser parser(jsonText);
    return parser.parseValue();
}
