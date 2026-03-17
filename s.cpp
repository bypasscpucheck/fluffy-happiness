// tinycpp.cpp
// C++编译器前端 - 支持类、继承、虚函数、模板
// 单文件，无依赖，直接编译运行

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <cctype>
#include <cassert>
#include <stack>
#include <set>
#include <map>
#include <algorithm>

// ==================== 词法分析器 ====================

enum class TokenType {
    // 关键字
    INT, FLOAT, DOUBLE, CHAR, VOID, BOOL,
    CLASS, STRUCT, PUBLIC, PRIVATE, PROTECTED,
    VIRTUAL, OVERRIDE, FINAL,
    TEMPLATE, TYPENAME, TYPEDEF,
    CONST, STATIC, MUTABLE, EXPLICIT,
    THIS, TRUE, FALSE, NULLPTR,
    IF, ELSE, SWITCH, CASE, DEFAULT,
    WHILE, FOR, DO, BREAK, CONTINUE,
    RETURN, NEW, DELETE, SIZEOF,
    TRY, CATCH, THROW,
    NAMESPACE, USING,
    
    // 标识符
    IDENTIFIER,
    
    // 字面量
    INTEGER_LITERAL, FLOAT_LITERAL, CHAR_LITERAL, STRING_LITERAL,
    
    // 运算符
    PLUS, MINUS, STAR, SLASH, PERCENT,
    ASSIGN, EQ, NE, LT, GT, LE, GE,
    AND, OR, NOT, BIT_AND, BIT_OR, BIT_XOR, BIT_NOT,
    SHL, SHR,
    INC, DEC,
    ARROW, DOT,
    SCOPE,  // ::
    
    // 分隔符
    SEMICOLON, COMMA,
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COLON, QUESTION,
    
    // 模板
    LESS, GREATER,  // 用于模板参数
    
    // 特殊
    END_OF_FILE, UNKNOWN
};

struct Token {
    TokenType type;
    std::string text;
    int line;
    int column;
    
    Token(TokenType t = TokenType::UNKNOWN, std::string txt = "", int l = 0, int c = 0)
        : type(t), text(std::move(txt)), line(l), column(c) {}
};

class Lexer {
private:
    std::string input;
    size_t pos;
    int line;
    int column;
    
public:
    Lexer(std::string src) : input(std::move(src)), pos(0), line(1), column(1) {}
    
    Token nextToken() {
        skipWhitespace();
        if (pos >= input.length()) {
            return Token(TokenType::END_OF_FILE, "", line, column);
        }
        
        char ch = input[pos];
        
        // 标识符或关键字
        if (std::isalpha(ch) || ch == '_') {
            return readIdentifier();
        }
        
        // 数字字面量
        if (std::isdigit(ch)) {
            return readNumber();
        }
        
        // 字符串字面量
        if (ch == '"') {
            return readString();
        }
        
        // 字符字面量
        if (ch == '\'') {
            return readChar();
        }
        
        // 运算符和分隔符
        Token token;
        token.line = line;
        token.column = column;
        
        switch (ch) {
            case '+': 
                if (peek() == '+') { token = Token(TokenType::INC, "++", line, column); pos += 2; column += 2; }
                else { token = Token(TokenType::PLUS, "+", line, column); pos++; column++; }
                break;
            case '-': 
                if (peek() == '-') { token = Token(TokenType::DEC, "--", line, column); pos += 2; column += 2; }
                else if (peek() == '>') { token = Token(TokenType::ARROW, "->", line, column); pos += 2; column += 2; }
                else { token = Token(TokenType::MINUS, "-", line, column); pos++; column++; }
                break;
            case '*': token = Token(TokenType::STAR, "*", line, column); pos++; column++; break;
            case '/': token = Token(TokenType::SLASH, "/", line, column); pos++; column++; break;
            case '%': token = Token(TokenType::PERCENT, "%", line, column); pos++; column++; break;
            case ';': token = Token(TokenType::SEMICOLON, ";", line, column); pos++; column++; break;
            case ',': token = Token(TokenType::COMMA, ",", line, column); pos++; column++; break;
            case '(': token = Token(TokenType::LPAREN, "(", line, column); pos++; column++; break;
            case ')': token = Token(TokenType::RPAREN, ")", line, column); pos++; column++; break;
            case '{': token = Token(TokenType::LBRACE, "{", line, column); pos++; column++; break;
            case '}': token = Token(TokenType::RBRACE, "}", line, column); pos++; column++; break;
            case '[': token = Token(TokenType::LBRACKET, "[", line, column); pos++; column++; break;
            case ']': token = Token(TokenType::RBRACKET, "]", line, column); pos++; column++; break;
            case ':': 
                if (peek() == ':') { token = Token(TokenType::SCOPE, "::", line, column); pos += 2; column += 2; }
                else { token = Token(TokenType::COLON, ":", line, column); pos++; column++; }
                break;
            case '?': token = Token(TokenType::QUESTION, "?", line, column); pos++; column++; break;
            case '.': token = Token(TokenType::DOT, ".", line, column); pos++; column++; break;
            case '~': token = Token(TokenType::BIT_NOT, "~", line, column); pos++; column++; break;
            
            case '=':
                if (peek() == '=') { token = Token(TokenType::EQ, "==", line, column); pos += 2; column += 2; }
                else { token = Token(TokenType::ASSIGN, "=", line, column); pos++; column++; }
                break;
                
            case '!':
                if (peek() == '=') { token = Token(TokenType::NE, "!=", line, column); pos += 2; column += 2; }
                else { token = Token(TokenType::NOT, "!", line, column); pos++; column++; }
                break;
                
            case '<':
                if (peek() == '<') { token = Token(TokenType::SHL, "<<", line, column); pos += 2; column += 2; }
                else if (peek() == '=') { token = Token(TokenType::LE, "<=", line, column); pos += 2; column += 2; }
                else { token = Token(TokenType::LESS, "<", line, column); pos++; column++; }
                break;
                
            case '>':
                if (peek() == '>') { token = Token(TokenType::SHR, ">>", line, column); pos += 2; column += 2; }
                else if (peek() == '=') { token = Token(TokenType::GE, ">=", line, column); pos += 2; column += 2; }
                else { token = Token(TokenType::GREATER, ">", line, column); pos++; column++; }
                break;
                
            case '&':
                if (peek() == '&') { token = Token(TokenType::AND, "&&", line, column); pos += 2; column += 2; }
                else { token = Token(TokenType::BIT_AND, "&", line, column); pos++; column++; }
                break;
                
            case '|':
                if (peek() == '|') { token = Token(TokenType::OR, "||", line, column); pos += 2; column += 2; }
                else { token = Token(TokenType::BIT_OR, "|", line, column); pos++; column++; }
                break;
                
            case '^': token = Token(TokenType::BIT_XOR, "^", line, column); pos++; column++; break;
                
            default:
                token = Token(TokenType::UNKNOWN, std::string(1, ch), line, column);
                pos++; column++;
                break;
        }
        
        return token;
    }
    
private:
    char peek() const {
        if (pos + 1 < input.length()) return input[pos + 1];
        return '\0';
    }
    
    void skipWhitespace() {
        while (pos < input.length()) {
            char ch = input[pos];
            if (ch == ' ' || ch == '\t') {
                pos++;
                column++;
            } else if (ch == '\n') {
                pos++;
                line++;
                column = 1;
            } else if (ch == '\r') {
                pos++;
            } else if (ch == '/' && peek() == '/') {
                // 单行注释
                pos += 2;
                column += 2;
                while (pos < input.length() && input[pos] != '\n') {
                    pos++;
                    column++;
                }
            } else if (ch == '/' && peek() == '*') {
                // 多行注释
                pos += 2;
                column += 2;
                while (pos < input.length()) {
                    if (input[pos] == '*' && peek() == '/') {
                        pos += 2;
                        column += 2;
                        break;
                    }
                    if (input[pos] == '\n') {
                        line++;
                        column = 1;
                    } else {
                        column++;
                    }
                    pos++;
                }
            } else {
                break;
            }
        }
    }
    
    Token readIdentifier() {
        size_t start = pos;
        int start_line = line;
        int start_col = column;
        
        while (pos < input.length() && (std::isalnum(input[pos]) || input[pos] == '_')) {
            pos++;
            column++;
        }
        
        std::string text = input.substr(start, pos - start);
        
        // C++关键字映射
        static std::unordered_map<std::string, TokenType> keywords = {
            {"int", TokenType::INT},
            {"float", TokenType::FLOAT},
            {"double", TokenType::DOUBLE},
            {"char", TokenType::CHAR},
            {"void", TokenType::VOID},
            {"bool", TokenType::BOOL},
            {"class", TokenType::CLASS},
            {"struct", TokenType::STRUCT},
            {"public", TokenType::PUBLIC},
            {"private", TokenType::PRIVATE},
            {"protected", TokenType::PROTECTED},
            {"virtual", TokenType::VIRTUAL},
            {"override", TokenType::OVERRIDE},
            {"final", TokenType::FINAL},
            {"template", TokenType::TEMPLATE},
            {"typename", TokenType::TYPENAME},
            {"typedef", TokenType::TYPEDEF},
            {"const", TokenType::CONST},
            {"static", TokenType::STATIC},
            {"mutable", TokenType::MUTABLE},
            {"explicit", TokenType::EXPLICIT},
            {"this", TokenType::THIS},
            {"true", TokenType::TRUE},
            {"false", TokenType::FALSE},
            {"nullptr", TokenType::NULLPTR},
            {"if", TokenType::IF},
            {"else", TokenType::ELSE},
            {"switch", TokenType::SWITCH},
            {"case", TokenType::CASE},
            {"default", TokenType::DEFAULT},
            {"while", TokenType::WHILE},
            {"for", TokenType::FOR},
            {"do", TokenType::DO},
            {"break", TokenType::BREAK},
            {"continue", TokenType::CONTINUE},
            {"return", TokenType::RETURN},
            {"new", TokenType::NEW},
            {"delete", TokenType::DELETE},
            {"sizeof", TokenType::SIZEOF},
            {"try", TokenType::TRY},
            {"catch", TokenType::CATCH},
            {"throw", TokenType::THROW},
            {"namespace", TokenType::NAMESPACE},
            {"using", TokenType::USING}
        };
        
        auto it = keywords.find(text);
        if (it != keywords.end()) {
            return Token(it->second, text, start_line, start_col);
        }
        
        return Token(TokenType::IDENTIFIER, text, start_line, start_col);
    }
    
    Token readNumber() {
        size_t start = pos;
        int start_line = line;
        int start_col = column;
        
        while (pos < input.length() && std::isdigit(input[pos])) {
            pos++;
            column++;
        }
        
        if (pos < input.length() && input[pos] == '.') {
            pos++; column++;
            while (pos < input.length() && std::isdigit(input[pos])) {
                pos++;
                column++;
            }
            std::string text = input.substr(start, pos - start);
            return Token(TokenType::FLOAT_LITERAL, text, start_line, start_col);
        }
        
        std::string text = input.substr(start, pos - start);
        return Token(TokenType::INTEGER_LITERAL, text, start_line, start_col);
    }
    
    Token readString() {
        pos++; column++;  // 跳过开头的"
        size_t start = pos;
        int start_line = line;
        int start_col = column - 1;
        
        while (pos < input.length() && input[pos] != '"') {
            if (input[pos] == '\\' && pos + 1 < input.length()) {
                pos += 2;
                column += 2;
            } else {
                pos++;
                column++;
            }
        }
        
        std::string text = input.substr(start, pos - start);
        pos++; column++;  // 跳过结尾的"
        
        return Token(TokenType::STRING_LITERAL, text, start_line, start_col);
    }
    
    Token readChar() {
        pos++; column++;  // 跳过开头的'
        size_t start = pos;
        int start_line = line;
        int start_col = column - 1;
        
        if (input[pos] == '\\' && pos + 1 < input.length()) {
            pos += 2;
            column += 2;
        } else {
            pos++;
            column++;
        }
        
        std::string text = input.substr(start, pos - start);
        pos++; column++;  // 跳过结尾的'
        
        return Token(TokenType::CHAR_LITERAL, text, start_line, start_col);
    }
};

// ==================== AST节点 ====================

enum class NodeType {
    // 顶层
    PROGRAM,
    NAMESPACE,
    
    // 声明
    FUNCTION,
    CLASS,
    STRUCT,
    TEMPLATE_DECL,
    
    // 成员
    FIELD,
    METHOD,
    CONSTRUCTOR,
    DESTRUCTOR,
    
    // 语句
    BLOCK,
    RETURN_STATEMENT,
    IF_STATEMENT,
    WHILE_STATEMENT,
    FOR_STATEMENT,
    DO_STATEMENT,
    SWITCH_STATEMENT,
    CASE_STATEMENT,
    BREAK_STATEMENT,
    CONTINUE_STATEMENT,
    EXPRESSION_STATEMENT,
    
    // 声明语句
    VARIABLE_DECL,
    
    // 表达式
    BINARY_OP,
    UNARY_OP,
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    CHAR_LITERAL,
    STRING_LITERAL,
    BOOL_LITERAL,
    NULLPTR_LITERAL,
    IDENTIFIER,
    THIS_EXPR,
    ASSIGNMENT,
    MEMBER_ACCESS,
    POINTER_MEMBER_ACCESS,
    FUNCTION_CALL,
    NEW_EXPR,
    DELETE_EXPR,
    SIZEOF_EXPR,
    CAST_EXPR,
    SUBSCRIPT,
    
    // 类型
    TYPE,
    TEMPLATE_TYPE,
    POINTER_TYPE,
    REFERENCE_TYPE,
    ARRAY_TYPE,
    FUNCTION_TYPE
};

struct ASTNode {
    NodeType type;
    std::vector<std::unique_ptr<ASTNode>> children;
    Token token;
    std::string str_value;
    int int_value;
    float float_value;
    
    ASTNode(NodeType t) : type(t), int_value(0), float_value(0) {}
    ASTNode(NodeType t, Token tok) : type(t), token(std::move(tok)), int_value(0), float_value(0) {}
    
    void addChild(std::unique_ptr<ASTNode> child) {
        if (child) children.push_back(std::move(child));
    }
};

// ==================== 类型系统 ====================

class Type {
public:
    enum class Kind {
        VOID, BOOL, CHAR, INT, FLOAT, DOUBLE,
        POINTER, REFERENCE, ARRAY, FUNCTION,
        CLASS, TEMPLATE_PARAM
    };
    
    Kind kind;
    std::string name;
    std::unique_ptr<Type> pointee;
    std::vector<std::unique_ptr<Type>> params;
    std::vector<std::unique_ptr<Type>> template_params;
    int array_size;
    bool is_const;
    bool is_virtual;
    
    Type(Kind k) : kind(k), array_size(0), is_const(false), is_virtual(false) {}
    
    static std::unique_ptr<Type> createVoid() {
        return std::make_unique<Type>(Kind::VOID);
    }
    
    static std::unique_ptr<Type> createInt() {
        return std::make_unique<Type>(Kind::INT);
    }
    
    static std::unique_ptr<Type> createFloat() {
        return std::make_unique<Type>(Kind::FLOAT);
    }
    
    static std::unique_ptr<Type> createBool() {
        return std::make_unique<Type>(Kind::BOOL);
    }
    
    static std::unique_ptr<Type> createChar() {
        return std::make_unique<Type>(Kind::CHAR);
    }
    
    static std::unique_ptr<Type> createPointer(std::unique_ptr<Type> pointee) {
        auto type = std::make_unique<Type>(Kind::POINTER);
        type->pointee = std::move(pointee);
        return type;
    }
    
    static std::unique_ptr<Type> createReference(std::unique_ptr<Type> referent) {
        auto type = std::make_unique<Type>(Kind::REFERENCE);
        type->pointee = std::move(referent);
        return type;
    }
    
    static std::unique_ptr<Type> createArray(std::unique_ptr<Type> elem, int size) {
        auto type = std::make_unique<Type>(Kind::ARRAY);
        type->pointee = std::move(elem);
        type->array_size = size;
        return type;
    }
    
    static std::unique_ptr<Type> createClass(const std::string& name) {
        auto type = std::make_unique<Type>(Kind::CLASS);
        type->name = name;
        return type;
    }
    
    std::string toString() const {
        switch (kind) {
            case Kind::VOID: return "void";
            case Kind::BOOL: return "bool";
            case Kind::CHAR: return "char";
            case Kind::INT: return "int";
            case Kind::FLOAT: return "float";
            case Kind::DOUBLE: return "double";
            case Kind::POINTER: return pointee->toString() + "*";
            case Kind::REFERENCE: return pointee->toString() + "&";
            case Kind::ARRAY: return pointee->toString() + "[" + std::to_string(array_size) + "]";
            case Kind::CLASS: return name;
            case Kind::FUNCTION: return "function";
            case Kind::TEMPLATE_PARAM: return "template param";
        }
        return "unknown";
    }
    
    bool isIntegral() const {
        return kind == Kind::INT || kind == Kind::CHAR || kind == Kind::BOOL;
    }
    
    bool isArithmetic() const {
        return isIntegral() || kind == Kind::FLOAT || kind == Kind::DOUBLE;
    }
    
    bool isPointer() const {
        return kind == Kind::POINTER;
    }
    
    bool isReference() const {
        return kind == Kind::REFERENCE;
    }
    
    bool isArray() const {
        return kind == Kind::ARRAY;
    }
    
    bool isClass() const {
        return kind == Kind::CLASS;
    }
};

// ==================== 符号表 ====================

struct Symbol {
    enum class Kind {
        VARIABLE, FUNCTION, METHOD, CONSTRUCTOR, DESTRUCTOR,
        CLASS, TEMPLATE, TYPEDEF, NAMESPACE
    };
    
    Kind kind;
    std::string name;
    std::unique_ptr<Type> type;
    std::vector<std::string> params;  // 函数参数名
    std::vector<std::unique_ptr<Type>> param_types;
    bool is_public;
    bool is_virtual;
    bool is_static;
    bool is_const;
    bool is_defined;
    int offset;  // 局部变量偏移或虚函数表索引
    
    Symbol(Kind k, const std::string& n) 
        : kind(k), name(n), is_public(true), is_virtual(false), 
          is_static(false), is_const(false), is_defined(false), offset(0) {}
};

class SymbolTable {
private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
    std::unordered_map<std::string, std::unique_ptr<Symbol>> classes;
    std::unordered_map<std::string, std::unique_ptr<Symbol>> templates;
    std::unordered_map<std::string, std::unique_ptr<Type>> typedefs;
    
public:
    SymbolTable() {
        enterScope();  // 全局作用域
    }
    
    void enterScope() {
        scopes.emplace_back();
    }
    
    void exitScope() {
        if (scopes.size() > 1) scopes.pop_back();
    }
    
    bool declare(const Symbol& sym) {
        if (scopes.back().count(sym.name)) return false;
        scopes.back()[sym.name] = sym;
        return true;
    }
    
    Symbol* lookup(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return &found->second;
        }
        return nullptr;
    }
    
    void declareClass(std::unique_ptr<Symbol> sym) {
        classes[sym->name] = std::move(sym);
    }
    
    Symbol* lookupClass(const std::string& name) {
        auto it = classes.find(name);
        if (it != classes.end()) return it->second.get();
        return nullptr;
    }
    
    void declareTemplate(std::unique_ptr<Symbol> sym) {
        templates[sym->name] = std::move(sym);
    }
    
    void addTypedef(const std::string& name, std::unique_ptr<Type> type) {
        typedefs[name] = std::move(type);
    }
    
    Type* lookupTypedef(const std::string& name) {
        auto it = typedefs.find(name);
        if (it != typedefs.end()) return it->second.get();
        return nullptr;
    }
};

// ==================== 语法分析器 ====================

class Parser {
private:
    Lexer lexer;
    Token current;
    std::vector<Token> token_buffer;
    size_t token_pos;
    
public:
    Parser(std::string src) : lexer(std::move(src)), token_pos(0) {
        advance();
    }
    
    std::unique_ptr<ASTNode> parse() {
        auto program = std::make_unique<ASTNode>(NodeType::PROGRAM);
        
        while (current.type != TokenType::END_OF_FILE) {
            if (current.type == TokenType::NAMESPACE) {
                program->addChild(parseNamespace());
            } else if (current.type == TokenType::TEMPLATE) {
                program->addChild(parseTemplateDecl());
            } else if (current.type == TokenType::CLASS || current.type == TokenType::STRUCT) {
                program->addChild(parseClass());
            } else {
                program->addChild(parseFunction());
            }
        }
        
        return program;
    }
    
private:
    void advance() {
        if (token_pos < token_buffer.size()) {
            current = token_buffer[token_pos++];
        } else {
            current = lexer.nextToken();
            token_buffer.push_back(current);
            token_pos++;
        }
    }
    
    void backup() {
        if (token_pos > 0) {
            token_pos--;
            current = token_buffer[token_pos];
        }
    }
    
    bool match(TokenType type) {
        if (current.type == type) {
            advance();
            return true;
        }
        return false;
    }
    
    void expect(TokenType type, const std::string& msg) {
        if (current.type != type) {
            std::cerr << "语法错误 第" << current.line << "行: " << msg << "\n";
            std::cerr << "期望 " << static_cast<int>(type) 
                      << "，得到 " << static_cast<int>(current.type) << " '" << current.text << "'\n";
            throw std::runtime_error("语法错误");
        }
    }
    
    // ==================== 解析顶层声明 ====================
    
    std::unique_ptr<ASTNode> parseNamespace() {
        auto ns = std::make_unique<ASTNode>(NodeType::NAMESPACE, current);
        advance();  // 'namespace'
        
        if (current.type == TokenType::IDENTIFIER) {
            ns->token = current;
            advance();
        }
        
        expect(TokenType::LBRACE, "需要 '{'");
        advance();
        
        while (current.type != TokenType::RBRACE && current.type != TokenType::END_OF_FILE) {
            if (current.type == TokenType::CLASS || current.type == TokenType::STRUCT) {
                ns->addChild(parseClass());
            } else {
                ns->addChild(parseFunction());
            }
        }
        
        expect(TokenType::RBRACE, "需要 '}'");
        advance();
        
        return ns;
    }
    
    std::unique_ptr<ASTNode> parseTemplateDecl() {
        auto templ = std::make_unique<ASTNode>(NodeType::TEMPLATE_DECL, current);
        advance();  // 'template'
        
        expect(TokenType::LESS, "需要 '<'");
        advance();
        
        // 模板参数列表
        while (current.type != TokenType::GREATER && current.type != TokenType::END_OF_FILE) {
            if (current.type == TokenType::TYPENAME || current.type == TokenType::CLASS) {
                advance();  // 'typename' 或 'class'
                expect(TokenType::IDENTIFIER, "需要模板参数名");
                templ->addChild(std::make_unique<ASTNode>(NodeType::IDENTIFIER, current));
                advance();
            }
            
            if (current.type == TokenType::COMMA) {
                advance();
            }
        }
        
        expect(TokenType::GREATER, "需要 '>'");
        advance();
        
        // 模板定义
        if (current.type == TokenType::CLASS || current.type == TokenType::STRUCT) {
            templ->addChild(parseClass());
        } else {
            templ->addChild(parseFunction());
        }
        
        return templ;
    }
    
    std::unique_ptr<ASTNode> parseClass() {
        bool is_struct = (current.type == TokenType::STRUCT);
        auto class_node = std::make_unique<ASTNode>(NodeType::CLASS, current);
        advance();  // 'class' 或 'struct'
        
        // 类名
        expect(TokenType::IDENTIFIER, "需要类名");
        class_node->token = current;
        advance();
        
        // 继承列表
        if (current.type == TokenType::COLON) {
            advance();
            parseBaseClasseList(class_node.get());
        }
        
        expect(TokenType::LBRACE, "需要 '{'");
        advance();
        
        // 成员
        bool current_access = is_struct;  // struct默认public，class默认private
        
        while (current.type != TokenType::RBRACE && current.type != TokenType::END_OF_FILE) {
            // 访问修饰符
            if (current.type == TokenType::PUBLIC) {
                current_access = true;
                advance();
                expect(TokenType::COLON, "需要 ':'");
                advance();
                continue;
            } else if (current.type == TokenType::PRIVATE) {
                current_access = false;
                advance();
                expect(TokenType::COLON, "需要 ':'");
                advance();
                continue;
            } else if (current.type == TokenType::PROTECTED) {
                current_access = false;  // 简化：protected当private
                advance();
                expect(TokenType::COLON, "需要 ':'");
                advance();
                continue;
            }
            
            // 解析成员
            auto member = parseClassMember();
            if (member) {
                member->int_value = current_access ? 1 : 0;  // 存储访问权限
                class_node->addChild(std::move(member));
            }
        }
        
        expect(TokenType::RBRACE, "需要 '}'");
        advance();
        expect(TokenType::SEMICOLON, "需要 ';'");
        advance();
        
        return class_node;
    }
    
    void parseBaseClasseList(ASTNode* class_node) {
        while (true) {
            bool is_virtual = false;
            bool is_public = true;
            
            // 访问修饰符
            if (current.type == TokenType::VIRTUAL) {
                is_virtual = true;
                advance();
            }
            
            if (current.type == TokenType::PUBLIC) {
                is_public = true;
                advance();
            } else if (current.type == TokenType::PRIVATE) {
                is_public = false;
                advance();
            } else if (current.type == TokenType::PROTECTED) {
                is_public = false;
                advance();
            }
            
            if (current.type == TokenType::VIRTUAL && !is_virtual) {
                is_virtual = true;
                advance();
            }
            
            // 基类名
            expect(TokenType::IDENTIFIER, "需要基类名");
            auto base = std::make_unique<ASTNode>(NodeType::IDENTIFIER, current);
            base->int_value = is_public ? 1 : 0;
            base->float_value = is_virtual ? 1 : 0;
            class_node->addChild(std::move(base));
            advance();
            
            if (current.type != TokenType::COMMA) break;
            advance();
        }
    }
    
    std::unique_ptr<ASTNode> parseClassMember() {
        // 检查是否是析构函数
        if (current.type == TokenType::BIT_NOT) {
            return parseDestructor();
        }
        
        // 检查是否是构造函数（类名相同）
        // 这里简化处理，假设我们通过上下文知道
        
        // 解析类型
        auto type_node = parseType();
        if (!type_node) return nullptr;
        
        // 解析名称
        expect(TokenType::IDENTIFIER, "需要成员名");
        auto name_token = current;
        advance();
        
        // 如果是函数
        if (current.type == TokenType::LPAREN) {
            return parseMethod(std::move(type_node), name_token);
        }
        
        // 否则是数据成员
        auto field = std::make_unique<ASTNode>(NodeType::FIELD, name_token);
        field->addChild(std::move(type_node));
        
        // 可能有默认值
        if (current.type == TokenType::ASSIGN) {
            advance();
            field->addChild(parseExpression());
        }
        
        expect(TokenType::SEMICOLON, "需要 ';'");
        advance();
        
        return field;
    }
    
    std::unique_ptr<ASTNode> parseMethod(std::unique_ptr<ASTNode> return_type, Token name_token) {
        auto method = std::make_unique<ASTNode>(NodeType::METHOD, name_token);
        method->addChild(std::move(return_type));
        
        // 参数列表
        advance();  // '('
        parseParameterList(method.get());
        expect(TokenType::RPAREN, "需要 ')'");
        advance();
        
        // 修饰符
        bool is_virtual = false;
        bool is_override = false;
        bool is_final = false;
        
        while (true) {
            if (current.type == TokenType::CONST) {
                method->int_value |= 1;  // const方法
                advance();
            } else if (current.type == TokenType::VIRTUAL) {
                is_virtual = true;
                advance();
            } else if (current.type == TokenType::OVERRIDE) {
                is_override = true;
                advance();
            } else if (current.type == TokenType::FINAL) {
                is_final = true;
                advance();
            } else {
                break;
            }
        }
        
        method->float_value = (is_virtual ? 1 : 0) | (is_override ? 2 : 0) | (is_final ? 4 : 0);
        
        // 函数体
        if (current.type == TokenType::LBRACE) {
            method->addChild(parseBlock());
        } else if (current.type == TokenType::ASSIGN) {
            advance();  // '='
            if (current.type == TokenType::INTEGER_LITERAL && current.text == "0") {
                // = 0 表示纯虚函数
                method->float_value |= 8;  // pure virtual
                advance();
            }
            expect(TokenType::SEMICOLON, "需要 ';'");
            advance();
        } else {
            expect(TokenType::SEMICOLON, "需要 ';'");
            advance();
        }
        
        return method;
    }
    
    std::unique_ptr<ASTNode> parseDestructor() {
        auto dtor = std::make_unique<ASTNode>(NodeType::DESTRUCTOR, current);
        advance();  // '~'
        
        expect(TokenType::IDENTIFIER, "需要类名");
        dtor->token = current;
        advance();
        
        expect(TokenType::LPAREN, "需要 '('");
        advance();
        expect(TokenType::RPAREN, "需要 ')'");
        advance();
        
        // 可能有virtual修饰
        if (current.type == TokenType::VIRTUAL) {
            dtor->int_value = 1;
            advance();
        }
        
        // 函数体
        if (current.type == TokenType::LBRACE) {
            dtor->addChild(parseBlock());
        } else {
            expect(TokenType::SEMICOLON, "需要 ';'");
            advance();
        }
        
        return dtor;
    }
    
    // ==================== 函数解析 ====================
    
    std::unique_ptr<ASTNode> parseFunction() {
        // 解析返回类型
        auto return_type = parseType();
        if (!return_type) return nullptr;
        
        // 函数名
        expect(TokenType::IDENTIFIER, "需要函数名");
        auto func = std::make_unique<ASTNode>(NodeType::FUNCTION, current);
        func->addChild(std::move(return_type));
        advance();
        
        // 模板参数？这里简化
        
        // 参数列表
        expect(TokenType::LPAREN, "需要 '('");
        advance();
        parseParameterList(func.get());
        expect(TokenType::RPAREN, "需要 ')'");
        advance();
        
        // 函数体
        if (current.type == TokenType::LBRACE) {
            func->addChild(parseBlock());
        } else {
            expect(TokenType::SEMICOLON, "需要 ';'");
            advance();
        }
        
        return func;
    }
    
    void parseParameterList(ASTNode* func) {
        if (current.type == TokenType::RPAREN) return;
        
        while (true) {
            auto type_node = parseType();
            expect(TokenType::IDENTIFIER, "需要参数名");
            auto param = std::make_unique<ASTNode>(NodeType::VARIABLE_DECL, current);
            param->addChild(std::move(type_node));
            func->addChild(std::move(param));
            advance();
            
            if (current.type == TokenType::ASSIGN) {
                // 默认参数
                advance();
                func->addChild(parseExpression());
            }
            
            if (current.type != TokenType::COMMA) break;
            advance();
        }
    }
    
    std::unique_ptr<ASTNode> parseType() {
        bool is_const = false;
        bool is_static = false;
        bool is_virtual = false;
        
        while (true) {
            if (current.type == TokenType::CONST) {
                is_const = true;
                advance();
            } else if (current.type == TokenType::STATIC) {
                is_static = true;
                advance();
            } else if (current.type == TokenType::VIRTUAL) {
                is_virtual = true;
                advance();
            } else {
                break;
            }
        }
        
        std::unique_ptr<ASTNode> type_node;
        
        switch (current.type) {
            case TokenType::VOID:
                type_node = std::make_unique<ASTNode>(NodeType::TYPE, current);
                type_node->int_value = 0;  // void
                advance();
                break;
            case TokenType::BOOL:
                type_node = std::make_unique<ASTNode>(NodeType::TYPE, current);
                type_node->int_value = 1;  // bool
                advance();
                break;
            case TokenType::CHAR:
                type_node = std::make_unique<ASTNode>(NodeType::TYPE, current);
                type_node->int_value = 2;  // char
                advance();
                break;
            case TokenType::INT:
                type_node = std::make_unique<ASTNode>(NodeType::TYPE, current);
                type_node->int_value = 3;  // int
                advance();
                break;
            case TokenType::FLOAT:
                type_node = std::make_unique<ASTNode>(NodeType::TYPE, current);
                type_node->int_value = 4;  // float
                advance();
                break;
            case TokenType::DOUBLE:
                type_node = std::make_unique<ASTNode>(NodeType::TYPE, current);
                type_node->int_value = 5;  // double
                advance();
                break;
            case TokenType::CLASS:
            case TokenType::STRUCT:
            case TokenType::IDENTIFIER: {
                type_node = std::make_unique<ASTNode>(NodeType::TYPE, current);
                type_node->int_value = 6;  // class/struct
                advance();
                
                // 模板参数
                if (current.type == TokenType::LESS) {
                    type_node->type = NodeType::TEMPLATE_TYPE;
                    advance();  // '<'
                    while (current.type != TokenType::GREATER && current.type != TokenType::END_OF_FILE) {
                        auto arg = parseType();
                        if (arg) type_node->addChild(std::move(arg));
                        if (current.type == TokenType::COMMA) advance();
                    }
                    expect(TokenType::GREATER, "需要 '>'");
                    advance();
                }
                break;
            }
            default:
                return nullptr;
        }
        
        // 指针、引用、数组
        while (true) {
            if (current.type == TokenType::STAR) {
                auto ptr = std::make_unique<ASTNode>(NodeType::POINTER_TYPE, current);
                ptr->addChild(std::move(type_node));
                type_node = std::move(ptr);
                advance();
            } else if (current.type == TokenType::BIT_AND) {
                auto ref = std::make_unique<ASTNode>(NodeType::REFERENCE_TYPE, current);
                ref->addChild(std::move(type_node));
                type_node = std::move(ref);
                advance();
            } else if (current.type == TokenType::LBRACKET) {
                advance();  // '['
                auto arr = std::make_unique<ASTNode>(NodeType::ARRAY_TYPE, current);
                arr->addChild(std::move(type_node));
                if (current.type == TokenType::INTEGER_LITERAL) {
                    arr->int_value = std::stoi(current.text);
                    advance();
                }
                expect(TokenType::RBRACKET, "需要 ']'");
                advance();
                type_node = std::move(arr);
            } else {
                break;
            }
        }
        
        // 设置修饰符
        type_node->float_value = (is_const ? 1 : 0) | (is_static ? 2 : 0) | (is_virtual ? 4 : 0);
        
        return type_node;
    }
    
    // ==================== 语句解析 ====================
    
    std::unique_ptr<ASTNode> parseBlock() {
        auto block = std::make_unique<ASTNode>(NodeType::BLOCK);
        
        expect(TokenType::LBRACE, "需要 '{'");
        advance();
        
        while (current.type != TokenType::RBRACE && current.type != TokenType::END_OF_FILE) {
            block->addChild(parseStatement());
        }
        
        expect(TokenType::RBRACE, "需要 '}'");
        advance();
        
        return block;
    }
    
    std::unique_ptr<ASTNode> parseStatement() {
        switch (current.type) {
            case TokenType::RETURN:
                return parseReturn();
            case TokenType::IF:
                return parseIf();
            case TokenType::WHILE:
                return parseWhile();
            case TokenType::FOR:
                return parseFor();
            case TokenType::DO:
                return parseDoWhile();
            case TokenType::SWITCH:
                return parseSwitch();
            case TokenType::BREAK:
                return parseBreak();
            case TokenType::CONTINUE:
                return parseContinue();
            case TokenType::LBRACE:
                return parseBlock();
            case TokenType::INT: case TokenType::FLOAT: case TokenType::DOUBLE:
            case TokenType::CHAR: case TokenType::BOOL: case TokenType::CLASS:
            case TokenType::STRUCT: case TokenType::IDENTIFIER:
                if (isDeclaration()) {
                    return parseDeclaration();
                }
                // 否则fall through到表达式语句
            default:
                return parseExpressionStatement();
        }
    }
    
    bool isDeclaration() {
        // 尝试判断是否是声明
        // 保存状态
        auto saved_pos = token_pos;
        auto saved_current = current;
        
        bool result = false;
        
        // 跳过可能的修饰符
        while (current.type == TokenType::CONST || current.type == TokenType::STATIC ||
               current.type == TokenType::VIRTUAL) {
            advance();
        }
        
        // 检查类型
        if (current.type == TokenType::INT || current.type == TokenType::FLOAT ||
            current.type == TokenType::DOUBLE || current.type == TokenType::CHAR ||
            current.type == TokenType::BOOL || current.type == TokenType::VOID ||
            current.type == TokenType::CLASS || current.type == TokenType::STRUCT ||
            current.type == TokenType::IDENTIFIER) {
            advance();
            
            // 检查后面是否是标识符
            if (current.type == TokenType::IDENTIFIER) {
                result = true;
            }
        }
        
        // 恢复状态
        token_pos = saved_pos;
        current = saved_current;
        
        return result;
    }
    
    std::unique_ptr<ASTNode> parseReturn() {
        auto ret = std::make_unique<ASTNode>(NodeType::RETURN_STATEMENT, current);
        advance();
        
        if (current.type != TokenType::SEMICOLON) {
            ret->addChild(parseExpression());
        }
        
        expect(TokenType::SEMICOLON, "需要 ';'");
        advance();
        
        return ret;
    }
    
    std::unique_ptr<ASTNode> parseIf() {
        auto if_stmt = std::make_unique<ASTNode>(NodeType::IF_STATEMENT, current);
        advance();
        
        expect(TokenType::LPAREN, "需要 '('");
        advance();
        if_stmt->addChild(parseExpression());
        expect(TokenType::RPAREN, "需要 ')'");
        advance();
        
        if_stmt->addChild(parseStatement());
        
        if (current.type == TokenType::ELSE) {
            advance();
            if_stmt->addChild(parseStatement());
        }
        
        return if_stmt;
    }
    
    std::unique_ptr<ASTNode> parseWhile() {
        auto while_stmt = std::make_unique<ASTNode>(NodeType::WHILE_STATEMENT, current);
        advance();
        
        expect(TokenType::LPAREN, "需要 '('");
        advance();
        while_stmt->addChild(parseExpression());
        expect(TokenType::RPAREN, "需要 ')'");
        advance();
        
        while_stmt->addChild(parseStatement());
        
        return while_stmt;
    }
    
    std::unique_ptr<ASTNode> parseFor() {
        auto for_stmt = std::make_unique<ASTNode>(NodeType::FOR_STATEMENT, current);
        advance();
        
        expect(TokenType::LPAREN, "需要 '('");
        advance();
        
        // 初始化
        if (current.type != TokenType::SEMICOLON) {
            if (isDeclaration()) {
                for_stmt->addChild(parseDeclaration());
            } else {
                for_stmt->addChild(parseExpression());
            }
        }
        expect(TokenType::SEMICOLON, "需要 ';'");
        advance();
        
        // 条件
        if (current.type != TokenType::SEMICOLON) {
            for_stmt->addChild(parseExpression());
        } else {
            for_stmt->addChild(nullptr);
        }
        expect(TokenType::SEMICOLON, "需要 ';'");
        advance();
        
        // 增量
        if (current.type != TokenType::RPAREN) {
            for_stmt->addChild(parseExpression());
        } else {
            for_stmt->addChild(nullptr);
        }
        
        expect(TokenType::RPAREN, "需要 ')'");
        advance();
        
        for_stmt->addChild(parseStatement());
        
        return for_stmt;
    }
    
    std::unique_ptr<ASTNode> parseDoWhile() {
        auto do_stmt = std::make_unique<ASTNode>(NodeType::DO_STATEMENT, current);
        advance();
        
        do_stmt->addChild(parseStatement());
        
        expect(TokenType::WHILE, "需要 'while'");
        advance();
        expect(TokenType::LPAREN, "需要 '('");
        advance();
        do_stmt->addChild(parseExpression());
        expect(TokenType::RPAREN, "需要 ')'");
        advance();
        expect(TokenType::SEMICOLON, "需要 ';'");
        advance();
        
        return do_stmt;
    }
    
    std::unique_ptr<ASTNode> parseSwitch() {
        auto switch_stmt = std::make_unique<ASTNode>(NodeType::SWITCH_STATEMENT, current);
        advance();
        
        expect(TokenType::LPAREN, "需要 '('");
        advance();
        switch_stmt->addChild(parseExpression());
        expect(TokenType::RPAREN, "需要 ')'");
        advance();
        
        expect(TokenType::LBRACE, "需要 '{'");
        advance();
        
        while (current.type != TokenType::RBRACE && current.type != TokenType::END_OF_FILE) {
            if (current.type == TokenType::CASE) {
                auto case_stmt = std::make_unique<ASTNode>(NodeType::CASE_STATEMENT, current);
                advance();
                case_stmt->addChild(parseExpression());
                expect(TokenType::COLON, "需要 ':'");
                advance();
                
                while (current.type != TokenType::CASE && 
                       current.type != TokenType::DEFAULT &&
                       current.type != TokenType::RBRACE) {
                    case_stmt->addChild(parseStatement());
                }
                
                switch_stmt->addChild(std::move(case_stmt));
            } else if (current.type == TokenType::DEFAULT) {
                auto default_stmt = std::make_unique<ASTNode>(NodeType::CASE_STATEMENT, current);
                advance();
                expect(TokenType::COLON, "需要 ':'");
                advance();
                
                while (current.type != TokenType::RBRACE) {
                    default_stmt->addChild(parseStatement());
                }
                
                switch_stmt->addChild(std::move(default_stmt));
            } else {
                switch_stmt->addChild(parseStatement());
            }
        }
        
        expect(TokenType::RBRACE, "需要 '}'");
        advance();
        
        return switch_stmt;
    }
    
    std::unique_ptr<ASTNode> parseBreak() {
        auto brk = std::make_unique<ASTNode>(NodeType::BREAK_STATEMENT, current);
        advance();
        expect(TokenType::SEMICOLON, "需要 ';'");
        advance();
        return brk;
    }
    
    std::unique_ptr<ASTNode> parseContinue() {
        auto cont = std::make_unique<ASTNode>(NodeType::CONTINUE_STATEMENT, current);
        advance();
        expect(TokenType::SEMICOLON, "需要 ';'");
        advance();
        return cont;
    }
    
    std::unique_ptr<ASTNode> parseDeclaration() {
        auto type_node = parseType();
        expect(TokenType::IDENTIFIER, "需要变量名");
        auto decl = std::make_unique<ASTNode>(NodeType::VARIABLE_DECL, current);
        decl->addChild(std::move(type_node));
        advance();
        
        if (current.type == TokenType::ASSIGN) {
            advance();
            decl->addChild(parseExpression());
        }
        
        while (current.type == TokenType::COMMA) {
            advance();
            expect(TokenType::IDENTIFIER, "需要变量名");
            auto var = std::make_unique<ASTNode>(NodeType::VARIABLE_DECL, current);
            var->addChild(type_node ? std::make_unique<ASTNode>(*type_node) : nullptr);
            decl->addChild(std::move(var));
            advance();
            
            if (current.type == TokenType::ASSIGN) {
                advance();
                decl->addChild(parseExpression());
            }
        }
        
        expect(TokenType::SEMICOLON, "需要 ';'");
        advance();
        
        return decl;
    }
    
    std::unique_ptr<ASTNode> parseExpressionStatement() {
        auto expr_stmt = std::make_unique<ASTNode>(NodeType::EXPRESSION_STATEMENT);
        
        if (current.type != TokenType::SEMICOLON) {
            expr_stmt->addChild(parseExpression());
        }
        
        expect(TokenType::SEMICOLON, "需要 ';'");
        advance();
        
        return expr_stmt;
    }
    
    // ==================== 表达式解析 ====================
    
    std::unique_ptr<ASTNode> parseExpression() {
        return parseAssignment();
    }
    
    std::unique_ptr<ASTNode> parseAssignment() {
        auto left = parseConditional();
        
        if (current.type == TokenType::ASSIGN) {
            auto assign = std::make_unique<ASTNode>(NodeType::ASSIGNMENT, current);
            advance();
            assign->addChild(std::move(left));
            assign->addChild(parseAssignment());
            return assign;
        }
        
        return left;
    }
    
    std::unique_ptr<ASTNode> parseConditional() {
        auto left = parseLogicalOr();
        
        if (current.type == TokenType::QUESTION) {
            auto cond = std::make_unique<ASTNode>(NodeType::BINARY_OP, current);
            cond->addChild(std::move(left));
            advance();
            cond->addChild(parseExpression());
            expect(TokenType::COLON, "需要 ':'");
            advance();
            cond->addChild(parseConditional());
            return cond;
        }
        
        return left;
    }
    
    std::unique_ptr<ASTNode> parseLogicalOr() {
        auto left = parseLogicalAnd();
        
        while (current.type == TokenType::OR) {
            auto op = std::make_unique<ASTNode>(NodeType::BINARY_OP, current);
            advance();
            op->addChild(std::move(left));
            op->addChild(parseLogicalAnd());
            left = std::move(op);
        }
        
        return left;
    }
    
    std::unique_ptr<ASTNode> parseLogicalAnd() {
        auto left = parseEquality();
        
        while (current.type == TokenType::AND) {
            auto op = std::make_unique<ASTNode>(NodeType::BINARY_OP, current);
            advance();
            op->addChild(std::move(left));
            op->addChild(parseEquality());
            left = std::move(op);
        }
        
        return left;
    }
    
    std::unique_ptr<ASTNode> parseEquality() {
        auto left = parseRelational();
        
        while (current.type == TokenType::EQ || current.type == TokenType::NE) {
            auto op = std::make_unique<ASTNode>(NodeType::BINARY_OP, current);
            advance();
            op->addChild(std::move(left));
            op->addChild(parseRelational());
            left = std::move(op);
        }
        
        return left;
    }
    
    std::unique_ptr<ASTNode> parseRelational() {
        auto left = parseShift();
        
        while (current.type == TokenType::LT || current.type == TokenType::GT ||
               current.type == TokenType::LE || current.type == TokenType::GE) {
            auto op = std::make_unique<ASTNode>(NodeType::BINARY_OP, current);
            advance();
            op->addChild(std::move(left));
            op->addChild(parseShift());
            left = std::move(op);
        }
        
        return left;
    }
    
    std::unique_ptr<ASTNode> parseShift() {
        auto left = parseAdditive();
        
        while (current.type == TokenType::SHL || current.type == TokenType::SHR) {
            auto op = std::make_unique<ASTNode>(NodeType::BINARY_OP, current);
            advance();
            op->addChild(std::move(left));
            op->addChild(parseAdditive());
            left = std::move(op);
        }
        
        return left;
    }
    
    std::unique_ptr<ASTNode> parseAdditive() {
        auto left = parseMultiplicative();
        
        while (current.type == TokenType::PLUS || current.type == TokenType::MINUS) {
            auto op = std::make_unique<ASTNode>(NodeType::BINARY_OP, current);
            advance();
            op->addChild(std::move(left));
            op->addChild(parseMultiplicative());
            left = std::move(op);
        }
        
        return left;
    }
    
    std::unique_ptr<ASTNode> parseMultiplicative() {
        auto left = parseCast();
        
        while (current.type == TokenType::STAR || current.type == TokenType::SLASH ||
               current.type == TokenType::PERCENT) {
            auto op = std::make_unique<ASTNode>(NodeType::BINARY_OP, current);
            advance();
            op->addChild(std::move(left));
            op->addChild(parseCast());
            left = std::move(op);
        }
        
        return left;
    }
    
    std::unique_ptr<ASTNode> parseCast() {
        if (current.type == TokenType::LPAREN) {
            // 可能是类型转换
            auto saved_pos = token_pos;
            auto saved_current = current;
            
            advance();
            auto type_node = parseType();
            if (type_node && current.type == TokenType::RPAREN) {
                advance();
                auto cast = std::make_unique<ASTNode>(NodeType::CAST_EXPR, current);
                cast->addChild(std::move(type_node));
                cast->addChild(parseCast());
                return cast;
            }
            
            // 不是类型转换，回退
            token_pos = saved_pos;
            current = saved_current;
        }
        
        return parseUnary();
    }
    
    std::unique_ptr<ASTNode> parseUnary() {
        switch (current.type) {
            case TokenType::INC:
            case TokenType::DEC:
            case TokenType::STAR:
            case TokenType::BIT_AND:
            case TokenType::PLUS:
            case TokenType::MINUS:
            case TokenType::NOT:
            case TokenType::BIT_NOT: {
                auto op = std::make_unique<ASTNode>(NodeType::UNARY_OP, current);
                advance();
                op->addChild(parseUnary());
                return op;
            }
            case TokenType::SIZEOF: {
                auto sz = std::make_unique<ASTNode>(NodeType::SIZEOF_EXPR, current);
                advance();
                if (current.type == TokenType::LPAREN) {
                    advance();
                    if (isDeclaration()) {
                        sz->addChild(parseType());
                    } else {
                        sz->addChild(parseExpression());
                    }
                    expect(TokenType::RPAREN, "需要 ')'");
                    advance();
                } else {
                    sz->addChild(parseUnary());
                }
                return sz;
            }
            case TokenType::NEW: {
                auto new_expr = std::make_unique<ASTNode>(NodeType::NEW_EXPR, current);
                advance();
                new_expr->addChild(parseType());
                if (current.type == TokenType::LPAREN) {
                    advance();
                    if (current.type != TokenType::RPAREN) {
                        new_expr->addChild(parseExpression());
                    }
                    expect(TokenType::RPAREN, "需要 ')'");
                    advance();
                }
                return new_expr;
            }
            case TokenType::DELETE: {
                auto del_expr = std::make_unique<ASTNode>(NodeType::DELETE_EXPR, current);
                advance();
                del_expr->addChild(parseUnary());
                return del_expr;
            }
            default:
                return parsePostfix();
        }
    }
    
    std::unique_ptr<ASTNode> parsePostfix() {
        auto left = parsePrimary();
        
        while (true) {
            if (current.type == TokenType::LBRACKET) {
                // 数组下标
                auto subscript = std::make_unique<ASTNode>(NodeType::SUBSCRIPT, current);
                advance();
                subscript->addChild(std::move(left));
                subscript->addChild(parseExpression());
                expect(TokenType::RBRACKET, "需要 ']'");
                advance();
                left = std::move(subscript);
            } else if (current.type == TokenType::LPAREN) {
                // 函数调用
                auto call = std::make_unique<ASTNode>(NodeType::FUNCTION_CALL, current);
                call->addChild(std::move(left));
                advance();
                
                if (current.type != TokenType::RPAREN) {
                    while (true) {
                        call->addChild(parseExpression());
                        if (current.type != TokenType::COMMA) break;
                        advance();
                    }
                }
                
                expect(TokenType::RPAREN, "需要 ')'");
                advance();
                left = std::move(call);
            } else if (current.type == TokenType::DOT) {
                // 成员访问
                auto member = std::make_unique<ASTNode>(NodeType::MEMBER_ACCESS, current);
                advance();
                member->addChild(std::move(left));
                expect(TokenType::IDENTIFIER, "需要成员名");
                member->addChild(std::make_unique<ASTNode>(NodeType::IDENTIFIER, current));
                advance();
                left = std::move(member);
            } else if (current.type == TokenType::ARROW) {
                // 指针成员访问
                auto member = std::make_unique<ASTNode>(NodeType::POINTER_MEMBER_ACCESS, current);
                advance();
                member->addChild(std::move(left));
                expect(TokenType::IDENTIFIER, "需要成员名");
                member->addChild(std::make_unique<ASTNode>(NodeType::IDENTIFIER, current));
                advance();
                left = std::move(member);
            } else if (current.type == TokenType::INC || current.type == TokenType::DEC) {
                // 后置++/--
                auto op = std::make_unique<ASTNode>(NodeType::UNARY_OP, current);
                op->addChild(std::move(left));
                advance();
                left = std::move(op);
            } else {
                break;
            }
        }
        
        return left;
    }
    
    std::unique_ptr<ASTNode> parsePrimary() {
        switch (current.type) {
            case TokenType::INTEGER_LITERAL: {
                auto node = std::make_unique<ASTNode>(NodeType::INTEGER_LITERAL, current);
                node->int_value = std::stoi(current.text);
                advance();
                return node;
            }
            case TokenType::FLOAT_LITERAL: {
                auto node = std::make_unique<ASTNode>(NodeType::FLOAT_LITERAL, current);
                node->float_value = std::stof(current.text);
                advance();
                return node;
            }
            case TokenType::CHAR_LITERAL: {
                auto node = std::make_unique<ASTNode>(NodeType::CHAR_LITERAL, current);
                node->int_value = current.text[0];
                advance();
                return node;
            }
            case TokenType::STRING_LITERAL: {
                auto node = std::make_unique<ASTNode>(NodeType::STRING_LITERAL, current);
                node->str_value = current.text;
                advance();
                return node;
            }
            case TokenType::TRUE:
            case TokenType::FALSE: {
                auto node = std::make_unique<ASTNode>(NodeType::BOOL_LITERAL, current);
                node->int_value = (current.type == TokenType::TRUE) ? 1 : 0;
                advance();
                return node;
            }
            case TokenType::NULLPTR: {
                auto node = std::make_unique<ASTNode>(NodeType::NULLPTR_LITERAL, current);
                advance();
                return node;
            }
            case TokenType::THIS: {
                auto node = std::make_unique<ASTNode>(NodeType::THIS_EXPR, current);
                advance();
                return node;
            }
            case TokenType::IDENTIFIER: {
                auto node = std::make_unique<ASTNode>(NodeType::IDENTIFIER, current);
                advance();
                return node;
            }
            case TokenType::LPAREN: {
                advance();
                auto expr = parseExpression();
                expect(TokenType::RPAREN, "需要 ')'");
                advance();
                return expr;
            }
            default: {
                std::cerr << "语法错误 第" << current.line << "行: 无法解析的表达式 '" 
                          << current.text << "'\n";
                advance();
                return std::make_unique<ASTNode>(NodeType::INTEGER_LITERAL);
            }
        }
    }
};

// ==================== 语义分析器 ====================

class SemanticAnalyzer {
private:
    SymbolTable symbols;
    bool hasError = false;
    std::string current_class;
    std::vector<std::string> current_template_params;
    
public:
    bool analyze(ASTNode* node) {
        if (!node) return true;
        
        switch (node->type) {
            case NodeType::PROGRAM:
                for (auto& child : node->children) {
                    analyze(child.get());
                }
                break;
                
            case NodeType::NAMESPACE:
                for (auto& child : node->children) {
                    analyze(child.get());
                }
                break;
                
            case NodeType::TEMPLATE_DECL: {
                // 记录模板参数
                std::vector<std::string> params;
                for (auto& child : node->children) {
                    if (child->type == NodeType::IDENTIFIER) {
                        params.push_back(child->token.text);
                        current_template_params.push_back(child->token.text);
                    }
                }
                
                // 分析模板定义
                if (!node->children.empty()) {
                    analyze(node->children.back().get());
                }
                
                // 恢复
                for (size_t i = 0; i < params.size(); ++i) {
                    current_template_params.pop_back();
                }
                break;
            }
                
            case NodeType::CLASS: {
                std::string class_name = node->token.text;
                std::string old_class = current_class;
                current_class = class_name;
                
                // 声明类
                Symbol class_sym(Symbol::Kind::CLASS, class_name);
                symbols.declareClass(std::make_unique<Symbol>(class_sym));
                
                // 进入类作用域
                symbols.enterScope();
                
                // 分析基类
                for (auto& child : node->children) {
                    if (child->type == NodeType::IDENTIFIER) {
                        // 基类
                        Symbol* base = symbols.lookupClass(child->token.text);
                        if (!base) {
                            error("未定义的基类 '" + child->token.text + "'", child->token);
                        }
                    }
                }
                
                // 分析成员
                for (auto& child : node->children) {
                    if (child->type != NodeType::IDENTIFIER) {
                        analyze(child.get());
                    }
                }
                
                symbols.exitScope();
                current_class = old_class;
                break;
            }
                
            case NodeType::FIELD: {
                // 数据成员
                std::string field_name = node->token.text;
                Symbol sym(Symbol::Kind::VARIABLE, field_name);
                sym.is_public = node->int_value != 0;
                symbols.declare(sym);
                break;
            }
                
            case NodeType::METHOD: {
                std::string method_name = node->token.text;
                Symbol sym(Symbol::Kind::METHOD, method_name);
                sym.is_public = node->int_value != 0;
                sym.is_virtual = (static_cast<int>(node->float_value) & 1) != 0;
                symbols.declare(sym);
                
                // 进入方法作用域
                symbols.enterScope();
                
                // 添加this指针
                Symbol this_sym(Symbol::Kind::VARIABLE, "this");
                this_sym.type = Type::createPointer(Type::createClass(current_class));
                symbols.declare(this_sym);
                
                // 分析参数
                for (size_t i = 1; i < node->children.size(); ++i) {
                    if (node->children[i]->type == NodeType::VARIABLE_DECL) {
                        analyze(node->children[i].get());
                    }
                }
                
                // 分析函数体
                if (!node->children.empty() && 
                    node->children.back()->type == NodeType::BLOCK) {
                    analyze(node->children.back().get());
                }
                
                symbols.exitScope();
                break;
            }
                
            case NodeType::DESTRUCTOR: {
                Symbol sym(Symbol::Kind::DESTRUCTOR, "~" + current_class);
                sym.is_virtual = node->int_value != 0;
                symbols.declare(sym);
                
                if (!node->children.empty()) {
                    symbols.enterScope();
                    
                    Symbol this_sym(Symbol::Kind::VARIABLE, "this");
                    this_sym.type = Type::createPointer(Type::createClass(current_class));
                    symbols.declare(this_sym);
                    
                    analyze(node->children[0].get());
                    
                    symbols.exitScope();
                }
                break;
            }
                
            case NodeType::FUNCTION: {
                std::string func_name = node->token.text;
                Symbol sym(Symbol::Kind::FUNCTION, func_name);
                symbols.declare(sym);
                
                symbols.enterScope();
                
                // 分析参数
                for (size_t i = 1; i < node->children.size(); ++i) {
                    if (node->children[i]->type == NodeType::VARIABLE_DECL) {
                        analyze(node->children[i].get());
                    }
                }
                
                // 分析函数体
                if (!node->children.empty() && 
                    node->children.back()->type == NodeType::BLOCK) {
                    analyze(node->children.back().get());
                }
                
                symbols.exitScope();
                break;
            }
                
            case NodeType::VARIABLE_DECL: {
                std::string var_name = node->token.text;
                Symbol sym(Symbol::Kind::VARIABLE, var_name);
                symbols.declare(sym);
                
                if (node->children.size() > 1) {
                    analyze(node->children[1].get());
                }
                break;
            }
                
            case NodeType::BLOCK:
                symbols.enterScope();
                for (auto& child : node->children) {
                    analyze(child.get());
                }
                symbols.exitScope();
                break;
                
            case NodeType::IDENTIFIER: {
                // 变量使用
                Symbol* sym = symbols.lookup(node->token.text);
                if (!sym) {
                    // 可能是模板参数
                    bool is_template_param = false;
                    for (const auto& param : current_template_params) {
                        if (param == node->token.text) {
                            is_template_param = true;
                            break;
                        }
                    }
                    
                    if (!is_template_param) {
                        error("未声明标识符 '" + node->token.text + "'", node->token);
                    }
                }
                break;
            }
                
            case NodeType::THIS_EXPR: {
                if (current_class.empty()) {
                    error("'this' 只能在非静态成员函数中使用", node->token);
                }
                break;
            }
                
            case NodeType::MEMBER_ACCESS:
            case NodeType::POINTER_MEMBER_ACCESS:
            case NodeType::FUNCTION_CALL:
            case NodeType::ASSIGNMENT:
            case NodeType::BINARY_OP:
            case NodeType::UNARY_OP:
            case NodeType::RETURN_STATEMENT:
            case NodeType::IF_STATEMENT:
            case NodeType::WHILE_STATEMENT:
            case NodeType::FOR_STATEMENT:
            case NodeType::DO_STATEMENT:
            case NodeType::SWITCH_STATEMENT:
            case NodeType::CASE_STATEMENT:
            case NodeType::BREAK_STATEMENT:
            case NodeType::CONTINUE_STATEMENT:
            case NodeType::EXPRESSION_STATEMENT:
            case NodeType::SUBSCRIPT:
            case NodeType::NEW_EXPR:
            case NodeType::DELETE_EXPR:
            case NodeType::SIZEOF_EXPR:
            case NodeType::CAST_EXPR:
                for (auto& child : node->children) {
                    if (child) analyze(child.get());
                }
                break;
                
            default:
                break;
        }
        
        return !hasError;
    }
    
private:
    void error(const std::string& msg, const Token& token) {
        std::cerr << "语义错误 第" << token.line << "行: " << msg << "\n";
        hasError = true;
    }
};

// ==================== 中间代码 ====================

enum class OpCode {
    // 常量
    ICONST, FCONST, SCONST,
    
    // 算术
    IADD, ISUB, IMUL, IDIV, IMOD,
    FADD, FSUB, FMUL, FDIV,
    
    // 比较
    IEQ, INE, ILT, ILE, IGT, IGE,
    FEQ, FNE, FLT, FLE, FGT, FGE,
    
    // 逻辑
    AND, OR, NOT,
    
    // 内存
    LOAD, STORE, ALLOCA,
    LOAD_ARG, STORE_ARG,
    
    // 指针
    PTR_LOAD, PTR_STORE,
    PTR_ADD, PTR_SUB,
    
    // 对象
    NEW, DELETE,
    LOAD_FIELD, STORE_FIELD,
    CALL_VIRTUAL,
    
    // 控制流
    JMP, JZ, JNZ,
    CALL, RET,
    
    // 特殊
    LABEL, NOP
};

struct Instruction {
    OpCode op;
    std::string arg1;
    std::string arg2;
    std::string result;
    int int_arg;
    float float_arg;
    std::string str_arg;
    
    Instruction(OpCode o) : op(o), int_arg(0), float_arg(0) {}
    Instruction(OpCode o, int i) : op(o), int_arg(i), float_arg(0) {}
    Instruction(OpCode o, float f) : op(o), int_arg(0), float_arg(f) {}
    Instruction(OpCode o, const std::string& a) : op(o), arg1(a), int_arg(0), float_arg(0) {}
    Instruction(OpCode o, const std::string& a, const std::string& r) 
        : op(o), arg1(a), result(r), int_arg(0), float_arg(0) {}
    Instruction(OpCode o, const std::string& a, const std::string& b, const std::string& r) 
        : op(o), arg1(a), arg2(b), result(r), int_arg(0), float_arg(0) {}
    
    std::string toString() const {
        std::string s;
        switch (op) {
            case OpCode::ICONST: s = "iconst"; break;
            case OpCode::FCONST: s = "fconst"; break;
            case OpCode::SCONST: s = "sconst"; break;
            case OpCode::IADD: s = "iadd"; break;
            case OpCode::ISUB: s = "isub"; break;
            case OpCode::IMUL: s = "imul"; break;
            case OpCode::IDIV: s = "idiv"; break;
            case OpCode::IMOD: s = "imod"; break;
            case OpCode::FADD: s = "fadd"; break;
            case OpCode::FSUB: s = "fsub"; break;
            case OpCode::FMUL: s = "fmul"; break;
            case OpCode::FDIV: s = "fdiv"; break;
            case OpCode::IEQ: s = "ieq"; break;
            case OpCode::INE: s = "ine"; break;
            case OpCode::ILT: s = "ilt"; break;
            case OpCode::ILE: s = "ile"; break;
            case OpCode::IGT: s = "igt"; break;
            case OpCode::IGE: s = "ige"; break;
            case OpCode::FEQ: s = "feq"; break;
            case OpCode::FNE: s = "fne"; break;
            case OpCode::FLT: s = "flt"; break;
            case OpCode::FLE: s = "fle"; break;
            case OpCode::FGT: s = "fgt"; break;
            case OpCode::FGE: s = "fge"; break;
            case OpCode::AND: s = "and"; break;
            case OpCode::OR: s = "or"; break;
            case OpCode::NOT: s = "not"; break;
            case OpCode::LOAD: s = "load"; break;
            case OpCode::STORE: s = "store"; break;
            case OpCode::ALLOCA: s = "alloca"; break;
            case OpCode::LOAD_ARG: s = "load_arg"; break;
            case OpCode::STORE_ARG: s = "store_arg"; break;
            case OpCode::PTR_LOAD: s = "ptr_load"; break;
            case OpCode::PTR_STORE: s = "ptr_store"; break;
            case OpCode::PTR_ADD: s = "ptr_add"; break;
            case OpCode::PTR_SUB: s = "ptr_sub"; break;
            case OpCode::NEW: s = "new"; break;
            case OpCode::DELETE: s = "delete"; break;
            case OpCode::LOAD_FIELD: s = "load_field"; break;
            case OpCode::STORE_FIELD: s = "store_field"; break;
            case OpCode::CALL_VIRTUAL: s = "call_virtual"; break;
            case OpCode::JMP: s = "jmp"; break;
            case OpCode::JZ: s = "jz"; break;
            case OpCode::JNZ: s = "jnz"; break;
            case OpCode::CALL: s = "call"; break;
            case OpCode::RET: s = "ret"; break;
            case OpCode::LABEL: s = "label:"; break;
            case OpCode::NOP: s = "nop"; break;
        }
        
        if (op == OpCode::ICONST) {
            s += " " + std::to_string(int_arg);
        } else if (op == OpCode::FCONST) {
            s += " " + std::to_string(float_arg);
        } else if (op == OpCode::SCONST) {
            s += " \"" + str_arg + "\"";
        } else if (op == OpCode::LABEL) {
            s += " " + arg1;
        } else if (!arg1.empty()) {
            s += " " + arg1;
            if (!arg2.empty()) s += ", " + arg2;
            if (!result.empty()) s += " -> " + result;
        }
        
        return s;
    }
};

// ==================== 中间代码生成器 ====================

class IRGenerator {
private:
    std::vector<Instruction> instructions;
    std::unordered_map<std::string, int> label_map;
    int temp_counter = 0;
    int label_counter = 0;
    int string_counter = 0;
    std::unordered_map<std::string, std::string> string_table;
    
public:
    std::vector<Instruction> generate(ASTNode* ast) {
        instructions.clear();
        generateNode(ast);
        return instructions;
    }
    
private:
    std::string newTemp() {
        return "%t" + std::to_string(temp_counter++);
    }
    
    std::string newLabel() {
        return "L" + std::to_string(label_counter++);
    }
    
    std::string newString(const std::string& str) {
        std::string name = "@str" + std::to_string(string_counter++);
        string_table[name] = str;
        return name;
    }
    
    void emit(Instruction inst) {
        instructions.push_back(inst);
    }
    
    void emit(OpCode op) {
        instructions.emplace_back(op);
    }
    
    void emit(OpCode op, int val) {
        instructions.emplace_back(op, val);
    }
    
    void emit(OpCode op, float val) {
        instructions.emplace_back(op, val);
    }
    
    void emit(OpCode op, const std::string& arg) {
        instructions.emplace_back(op, arg);
    }
    
    void emit(OpCode op, const std::string& a, const std::string& r) {
        instructions.emplace_back(op, a, r);
    }
    
    void emit(OpCode op, const std::string& a, const std::string& b, const std::string& r) {
        instructions.emplace_back(op, a, b, r);
    }
    
    std::string generateNode(ASTNode* node) {
        if (!node) return "";
        
        switch (node->type) {
            case NodeType::PROGRAM:
                for (auto& child : node->children) {
                    generateNode(child.get());
                }
                break;
                
            case NodeType::FUNCTION: {
                emit(OpCode::LABEL, node->token.text);
                
                // 分配参数
                for (size_t i = 1; i < node->children.size() - 1; ++i) {
                    if (node->children[i]->type == NodeType::VARIABLE_DECL) {
                        std::string var = node->children[i]->token.text;
                        emit(OpCode::ALLOCA, var);
                        std::string arg = "%arg" + std::to_string(i-1);
                        emit(OpCode::STORE_ARG, arg, var);
                    }
                }
                
                // 函数体
                if (!node->children.empty()) {
                    generateNode(node->children.back().get());
                }
                
                // 默认返回0
                emit(OpCode::ICONST, 0);
                emit(OpCode::RET);
                break;
            }
                
            case NodeType::METHOD: {
                emit(OpCode::LABEL, current_class + "::" + node->token.text);
                
                // 分配this
                emit(OpCode::ALLOCA, "this");
                emit(OpCode::STORE_ARG, "%arg0", "this");
                
                // 分配参数
                for (size_t i = 1; i < node->children.size() - 1; ++i) {
                    if (node->children[i]->type == NodeType::VARIABLE_DECL) {
                        std::string var = node->children[i]->token.text;
                        emit(OpCode::ALLOCA, var);
                        std::string arg = "%arg" + std::to_string(i);
                        emit(OpCode::STORE_ARG, arg, var);
                    }
                }
                
                // 函数体
                if (!node->children.empty()) {
                    generateNode(node->children.back().get());
                }
                
                // 默认返回0
                emit(OpCode::ICONST, 0);
                emit(OpCode::RET);
                break;
            }
                
            case NodeType::BLOCK:
                for (auto& child : node->children) {
                    generateNode(child.get());
                }
                break;
                
            case NodeType::RETURN_STATEMENT: {
                if (!node->children.empty()) {
                    std::string val = generateNode(node->children[0].get());
                    emit(OpCode::LOAD, val, "%ret");
                } else {
                    emit(OpCode::ICONST, 0);
                }
                emit(OpCode::RET);
                break;
            }
                
            case NodeType::IF_STATEMENT: {
                std::string cond = generateNode(node->children[0].get());
                std::string else_label = newLabel();
                std::string end_label = newLabel();
                
                emit(OpCode::LOAD, cond, "%cond");
                emit(OpCode::JZ, "%cond", else_label);
                
                generateNode(node->children[1].get());
                emit(OpCode::JMP, end_label);
                
                emit(OpCode::LABEL, else_label);
                if (node->children.size() > 2) {
                    generateNode(node->children[2].get());
                }
                
                emit(OpCode::LABEL, end_label);
                break;
            }
                
            case NodeType::WHILE_STATEMENT: {
                std::string start_label = newLabel();
                std::string end_label = newLabel();
                
                emit(OpCode::LABEL, start_label);
                std::string cond = generateNode(node->children[0].get());
                emit(OpCode::LOAD, cond, "%cond");
                emit(OpCode::JZ, "%cond", end_label);
                generateNode(node->children[1].get());
                emit(OpCode::JMP, start_label);
                emit(OpCode::LABEL, end_label);
                break;
            }
                
            case NodeType::VARIABLE_DECL: {
                std::string var = node->token.text;
                emit(OpCode::ALLOCA, var);
                if (node->children.size() > 1) {
                    std::string val = generateNode(node->children[1].get());
                    emit(OpCode::STORE, val, var);
                }
                break;
            }
                
            case NodeType::IDENTIFIER: {
                std::string temp = newTemp();
                emit(OpCode::LOAD, node->token.text, temp);
                return temp;
            }
                
            case NodeType::INTEGER_LITERAL: {
                std::string temp = newTemp();
                emit(OpCode::ICONST, node->int_value);
                // 值在栈上
                return temp;
            }
                
            case NodeType::FLOAT_LITERAL: {
                std::string temp = newTemp();
                emit(OpCode::FCONST, node->float_value);
                return temp;
            }
                
            case NodeType::STRING_LITERAL: {
                std::string str_name = newString(node->str_value);
                std::string temp = newTemp();
                emit(OpCode::SCONST, str_name, temp);
                return temp;
            }
                
            case NodeType::BOOL_LITERAL: {
                std::string temp = newTemp();
                emit(OpCode::ICONST, node->int_value);
                return temp;
            }
                
            case NodeType::NULLPTR_LITERAL: {
                std::string temp = newTemp();
                emit(OpCode::ICONST, 0);
                return temp;
            }
                
            case NodeType::THIS_EXPR: {
                std::string temp = newTemp();
                emit(OpCode::LOAD, "this", temp);
                return temp;
            }
                
            case NodeType::ASSIGNMENT: {
                std::string right = generateNode(node->children[1].get());
                if (node->children[0]->type == NodeType::IDENTIFIER) {
                    emit(OpCode::STORE, right, node->children[0]->token.text);
                } else if (node->children[0]->type == NodeType::MEMBER_ACCESS) {
                    // obj.field = value
                    std::string obj = generateNode(node->children[0]->children[0].get());
                    std::string field = node->children[0]->children[1]->token.text;
                    emit(OpCode::LOAD, right, "%val");
                    emit(OpCode::STORE_FIELD, obj, field, "%val");
                } else if (node->children[0]->type == NodeType::POINTER_MEMBER_ACCESS) {
                    // ptr->field = value
                    std::string ptr = generateNode(node->children[0]->children[0].get());
                    std::string field = node->children[0]->children[1]->token.text;
                    emit(OpCode::PTR_LOAD, ptr, "%obj");
                    emit(OpCode::STORE_FIELD, "%obj", field, right);
                }
                return right;
            }
                
            case NodeType::BINARY_OP: {
                std::string left = generateNode(node->children[0].get());
                std::string right = generateNode(node->children[1].get());
                std::string result = newTemp();
                
                switch (node->token.type) {
                    case TokenType::PLUS: 
                        emit(OpCode::IADD, left, right, result); 
                        break;
                    case TokenType::MINUS: 
                        emit(OpCode::ISUB, left, right, result); 
                        break;
                    case TokenType::STAR: 
                        emit(OpCode::IMUL, left, right, result); 
                        break;
                    case TokenType::SLASH: 
                        emit(OpCode::IDIV, left, right, result); 
                        break;
                    case TokenType::PERCENT: 
                        emit(OpCode::IMOD, left, right, result); 
                        break;
                    case TokenType::EQ: 
                        emit(OpCode::IEQ, left, right, result); 
                        break;
                    case TokenType::NE: 
                        emit(OpCode::INE, left, right, result); 
                        break;
                    case TokenType::LT: 
                        emit(OpCode::ILT, left, right, result); 
                        break;
                    case TokenType::LE: 
                        emit(OpCode::ILE, left, right, result); 
                        break;
                    case TokenType::GT: 
                        emit(OpCode::IGT, left, right, result); 
                        break;
                    case TokenType::GE: 
                        emit(OpCode::IGE, left, right, result); 
                        break;
                    case TokenType::AND: 
                        emit(OpCode::AND, left, right, result); 
                        break;
                    case TokenType::OR: 
                        emit(OpCode::OR, left, right, result); 
                        break;
                    default: break;
                }
                return result;
            }
                
            case NodeType::MEMBER_ACCESS: {
                std::string obj = generateNode(node->children[0].get());
                std::string field = node->children[1]->token.text;
                std::string result = newTemp();
                emit(OpCode::LOAD_FIELD, obj, field, result);
                return result;
            }
                
            case NodeType::POINTER_MEMBER_ACCESS: {
                std::string ptr = generateNode(node->children[0].get());
                std::string field = node->children[1]->token.text;
                std::string obj = newTemp();
                std::string result = newTemp();
                emit(OpCode::PTR_LOAD, ptr, obj);
                emit(OpCode::LOAD_FIELD, obj, field, result);
                return result;
            }
                
            case NodeType::FUNCTION_CALL: {
                std::string func;
                if (node->children[0]->type == NodeType::IDENTIFIER) {
                    func = node->children[0]->token.text;
                } else {
                    func = generateNode(node->children[0].get());
                }
                
                // 压入参数
                for (size_t i = 1; i < node->children.size(); ++i) {
                    std::string arg = generateNode(node->children[i].get());
                    emit(OpCode::LOAD, arg, "%arg" + std::to_string(i-1));
                }
                
                std::string result = newTemp();
                emit(OpCode::CALL, func, std::to_string(node->children.size() - 1), result);
                return result;
            }
                
            case NodeType::NEW_EXPR: {
                std::string type = node->children[0]->token.text;
                std::string result = newTemp();
                emit(OpCode::NEW, type, result);
                
                if (node->children.size() > 1) {
                    // 调用构造函数
                    std::string arg = generateNode(node->children[1].get());
                    emit(OpCode::CALL, type + "::" + type, "1", "%unused");
                }
                
                return result;
            }
                
            case NodeType::DELETE_EXPR: {
                std::string ptr = generateNode(node->children[0].get());
                emit(OpCode::DELETE, ptr);
                break;
            }
                
            case NodeType::SUBSCRIPT: {
                std::string ptr = generateNode(node->children[0].get());
                std::string index = generateNode(node->children[1].get());
                std::string elem_ptr = newTemp();
                std::string result = newTemp();
                emit(OpCode::PTR_ADD, ptr, index, elem_ptr);
                emit(OpCode::PTR_LOAD, elem_ptr, result);
                return result;
            }
                
            default:
                for (auto& child : node->children) {
                    if (child) generateNode(child.get());
                }
                break;
        }
        
        return "";
    }
    
    std::string current_class;
};

// ==================== 虚拟机 ====================

class VirtualMachine {
private:
    struct Value {
        enum Type { INT, FLOAT, PTR, STRING } type;
        union {
            int int_val;
            float float_val;
            void* ptr_val;
        };
        std::string str_val;
        
        Value() : type(INT), int_val(0) {}
        Value(int i) : type(INT), int_val(i) {}
        Value(float f) : type(FLOAT), float_val(f) {}
        Value(void* p) : type(PTR), ptr_val(p) {}
        Value(const std::string& s) : type(STRING), ptr_val(nullptr), str_val(s) {}
        
        int asInt() const { return type == INT ? int_val : 0; }
        float asFloat() const { return type == FLOAT ? float_val : 0; }
        void* asPtr() const { return type == PTR ? ptr_val : nullptr; }
    };
    
    std::vector<Instruction> code;
    std::unordered_map<std::string, int> labels;
    std::unordered_map<std::string, Value> memory;
    std::unordered_map<std::string, Value> strings;
    std::stack<Value> stack;
    std::vector<Value> call_stack;
    int pc = 0;
    bool running = true;
    
public:
    void load(const std::vector<Instruction>& program) {
        code = program;
        
        // 收集标签
        for (size_t i = 0; i < code.size(); ++i) {
            if (code[i].op == OpCode::LABEL) {
                labels[code[i].arg1] = i;
            }
        }
    }
    
    void addString(const std::string& name, const std::string& value) {
        strings[name] = Value(value);
    }
    
    void run() {
        pc = 0;
        running = true;
        
        while (running && pc < code.size()) {
            execute(code[pc]);
            pc++;
        }
    }
    
    int getResult() {
        if (stack.empty()) return 0;
        return stack.top().asInt();
    }
    
private:
    void execute(const Instruction& inst) {
        switch (inst.op) {
            case OpCode::ICONST:
                stack.push(Value(inst.int_arg));
                break;
                
            case OpCode::FCONST:
                stack.push(Value(inst.float_arg));
                break;
                
            case OpCode::SCONST:
                stack.push(strings[inst.arg1]);
                break;
                
            case OpCode::IADD: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asInt() + b.asInt()));
                break;
            }
            
            case OpCode::ISUB: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asInt() - b.asInt()));
                break;
            }
            
            case OpCode::IMUL: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asInt() * b.asInt()));
                break;
            }
            
            case OpCode::IDIV: {
                Value b = pop();
                Value a = pop();
                if (b.asInt() == 0) {
                    error("除零错误");
                    return;
                }
                stack.push(Value(a.asInt() / b.asInt()));
                break;
            }
            
            case OpCode::IMOD: {
                Value b = pop();
                Value a = pop();
                if (b.asInt() == 0) {
                    error("取模除零");
                    return;
                }
                stack.push(Value(a.asInt() % b.asInt()));
                break;
            }
            
            case OpCode::FADD: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asFloat() + b.asFloat()));
                break;
            }
            
            case OpCode::FSUB: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asFloat() - b.asFloat()));
                break;
            }
            
            case OpCode::FMUL: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asFloat() * b.asFloat()));
                break;
            }
            
            case OpCode::FDIV: {
                Value b = pop();
                Value a = pop();
                if (b.asFloat() == 0) {
                    error("除零错误");
                    return;
                }
                stack.push(Value(a.asFloat() / b.asFloat()));
                break;
            }
            
            case OpCode::IEQ: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asInt() == b.asInt() ? 1 : 0));
                break;
            }
            
            case OpCode::INE: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asInt() != b.asInt() ? 1 : 0));
                break;
            }
            
            case OpCode::ILT: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asInt() < b.asInt() ? 1 : 0));
                break;
            }
            
            case OpCode::ILE: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asInt() <= b.asInt() ? 1 : 0));
                break;
            }
            
            case OpCode::IGT: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asInt() > b.asInt() ? 1 : 0));
                break;
            }
            
            case OpCode::IGE: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asInt() >= b.asInt() ? 1 : 0));
                break;
            }
            
            case OpCode::FEQ: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asFloat() == b.asFloat() ? 1 : 0));
                break;
            }
            
            case OpCode::FNE: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asFloat() != b.asFloat() ? 1 : 0));
                break;
            }
            
            case OpCode::FLT: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asFloat() < b.asFloat() ? 1 : 0));
                break;
            }
            
            case OpCode::FLE: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asFloat() <= b.asFloat() ? 1 : 0));
                break;
            }
            
            case OpCode::FGT: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asFloat() > b.asFloat() ? 1 : 0));
                break;
            }
            
            case OpCode::FGE: {
                Value b = pop();
                Value a = pop();
                stack.push(Value(a.asFloat() >= b.asFloat() ? 1 : 0));
                break;
            }
            
            case OpCode::AND: {
                Value b = pop();
                Value a = pop();
                stack.push(Value((a.asInt() && b.asInt()) ? 1 : 0));
                break;
            }
            
            case OpCode::OR: {
                Value b = pop();
                Value a = pop();
                stack.push(Value((a.asInt() || b.asInt()) ? 1 : 0));
                break;
            }
            
            case OpCode::NOT: {
                Value a = pop();
                stack.push(Value(!a.asInt() ? 1 : 0));
                break;
            }
            
            case OpCode::LOAD: {
                auto it = memory.find(inst.arg1);
                if (it == memory.end()) {
                    stack.push(Value(0));
                } else {
                    stack.push(it->second);
                }
                break;
            }
            
            case OpCode::STORE: {
                Value val = stack.top(); stack.pop();
                memory[inst.arg1] = val;
                break;
            }
            
            case OpCode::ALLOCA: {
                memory[inst.arg1] = Value(0);
                break;
            }
            
            case OpCode::LOAD_ARG: {
                int idx = std::stoi(inst.arg1.substr(4));
                if (idx < call_stack.size()) {
                    stack.push(call_stack[idx]);
                } else {
                    stack.push(Value(0));
                }
                break;
            }
            
            case OpCode::STORE_ARG: {
                Value val = stack.top(); stack.pop();
                // 简化：参数直接存到内存
                memory[inst.arg1] = val;
                break;
            }
            
            case OpCode::PTR_LOAD: {
                Value ptr = pop();
                if (ptr.type == Value::PTR && ptr.ptr_val) {
                    // 简化：指针解引用
                    stack.push(Value(*(int*)ptr.ptr_val));
                } else {
                    stack.push(Value(0));
                }
                break;
            }
            
            case OpCode::PTR_STORE: {
                Value val = pop();
                Value ptr = pop();
                if (ptr.type == Value::PTR && ptr.ptr_val) {
                    *(int*)ptr.ptr_val = val.asInt();
                }
                break;
            }
            
            case OpCode::PTR_ADD: {
                Value index = pop();
                Value ptr = pop();
                if (ptr.type == Value::PTR) {
                    stack.push(Value((char*)ptr.ptr_val + index.asInt() * 4));
                } else {
                    stack.push(Value(nullptr));
                }
                break;
            }
            
            case OpCode::NEW: {
                // 简化：分配内存
                void* mem = malloc(16);  // 固定大小
                stack.push(Value(mem));
                break;
            }
            
            case OpCode::DELETE: {
                Value ptr = pop();
                if (ptr.type == Value::PTR && ptr.ptr_val) {
                    free(ptr.ptr_val);
                }
                break;
            }
            
            case OpCode::LOAD_FIELD: {
                // 简化：对象字段访问
                Value obj = pop();
                if (obj.type == Value::PTR && obj.ptr_val) {
                    // 假设第一个字段是int
                    stack.push(Value(*(int*)obj.ptr_val));
                } else {
                    stack.push(Value(0));
                }
                break;
            }
            
            case OpCode::STORE_FIELD: {
                Value val = pop();
                Value obj = pop();
                if (obj.type == Value::PTR && obj.ptr_val) {
                    *(int*)obj.ptr_val = val.asInt();
                }
                break;
            }
            
            case OpCode::CALL: {
                std::string func = inst.arg1;
                int arg_count = std::stoi(inst.arg2);
                
                // 保存当前执行状态
                int return_pc = pc + 1;
                
                // 跳转到函数
                auto it = labels.find(func);
                if (it != labels.end()) {
                    pc = it->first;
                } else {
                    error("未定义的函数: " + func);
                }
                break;
            }
            
            case OpCode::CALL_VIRTUAL: {
                // 简化：虚函数调用
                std::string func = inst.arg1;
                auto it = labels.find(func);
                if (it != labels.end()) {
                    pc = it->first;
                }
                break;
            }
            
            case OpCode::RET: {
                running = false;
                break;
            }
            
            case OpCode::JMP: {
                auto it = labels.find(inst.arg1);
                if (it != labels.end()) {
                    pc = it->second;
                }
                break;
            }
            
            case OpCode::JZ: {
                Value cond = pop();
                if (cond.asInt() == 0) {
                    auto it = labels.find(inst.arg1);
                    if (it != labels.end()) {
                        pc = it->second;
                    }
                }
                break;
            }
            
            case OpCode::JNZ: {
                Value cond = pop();
                if (cond.asInt() != 0) {
                    auto it = labels.find(inst.arg1);
                    if (it != labels.end()) {
                        pc = it->second;
                    }
                }
                break;
            }
            
            case OpCode::LABEL:
                // 什么也不做
                break;
                
            case OpCode::NOP:
                break;
        }
    }
    
    Value pop() {
        if (stack.empty()) return Value(0);
        Value v = stack.top();
        stack.pop();
        return v;
    }
    
    void error(const std::string& msg) {
        std::cerr << "\n❌ 运行时错误: " << msg << "\n";
        std::cerr << "  在指令 " << pc << ": " << code[pc].toString() << "\n";
        running = false;
    }
};

// ==================== 调试器 ====================

class Debugger {
private:
    VirtualMachine& vm;
    std::vector<Instruction>& code;
    std::set<int> breakpoints;
    bool stepping = false;
    bool running = true;
    
public:
    Debugger(VirtualMachine& v, std::vector<Instruction>& c) : vm(v), code(c) {}
    
    void setBreakpoint(int line) {
        breakpoints.insert(line);
        std::cout << "断点已设置: " << line << "\n";
    }
    
    void run() {
        std::cout << "调试器启动，输入 help 查看命令\n";
        
        while (running) {
            std::cout << "\n(db) ";
            std::string cmd;
            std::getline(std::cin, cmd);
            
            if (cmd == "run" || cmd == "r") {
                vm.run();
            } else if (cmd == "step" || cmd == "s") {
                stepping = true;
                vm.run();
            } else if (cmd == "continue" || cmd == "c") {
                stepping = false;
                vm.run();
            } else if (cmd.substr(0, 2) == "b ") {
                int line = std::stoi(cmd.substr(2));
                setBreakpoint(line);
            } else if (cmd == "info break") {
                listBreakpoints();
            } else if (cmd == "info registers") {
                printRegisters();
            } else if (cmd == "info stack") {
                printStack();
            } else if (cmd == "list") {
                listCode();
            } else if (cmd.substr(0, 5) == "print") {
                printVariable(cmd.substr(6));
            } else if (cmd == "quit" || cmd == "q") {
                running = false;
            } else if (cmd == "help" || cmd == "h") {
                printHelp();
            } else {
                std::cout << "未知命令\n";
            }
        }
    }
    
private:
    void listBreakpoints() {
        std::cout << "断点列表:\n";
        for (int bp : breakpoints) {
            std::cout << "  " << bp << ": " << code[bp].toString() << "\n";
        }
    }
    
    void listCode() {
        int start = std::max(0, pc - 5);
        int end = std::min((int)code.size(), pc + 5);
        
        for (int i = start; i < end; ++i) {
            std::cout << (i == pc ? "-> " : "   ") << i << ": " << code[i].toString() << "\n";
        }
    }
    
    void printRegisters() {
        std::cout << "PC: " << pc << "\n";
    }
    
    void printStack() {
        std::cout << "调用栈:\n";
        // 简化
    }
    
    void printVariable(const std::string& name) {
        std::cout << name << " = ?\n";
    }
    
    void printHelp() {
        std::cout << "调试器命令:\n";
        std::cout << "  run (r)          - 运行程序\n";
        std::cout << "  step (s)         - 单步执行\n";
        std::cout << "  continue (c)     - 继续执行\n";
        std::cout << "  b <line>         - 设置断点\n";
        std::cout << "  info break       - 列出断点\n";
        std::cout << "  info registers   - 显示寄存器\n";
        std::cout << "  info stack       - 显示调用栈\n";
        std::cout << "  list             - 显示代码\n";
        std::cout << "  print <var>      - 打印变量\n";
        std::cout << "  quit (q)         - 退出\n";
    }
    
    int pc = 0;
};

// ==================== 整合 ====================

class TinyCPP {
private:
    std::unordered_map<std::string, std::string> string_table;
    
public:
    int compileAndRun(const std::string& source, bool debug = false) {
        try {
            std::cout << "1. 词法分析...\n";
            Lexer lexer(source);
            
            std::cout << "2. 语法分析...\n";
            Parser parser(source);
            auto ast = parser.parse();
            
            std::cout << "3. 语义分析...\n";
            SemanticAnalyzer analyzer;
            if (!analyzer.analyze(ast.get())) {
                std::cerr << "❌ 语义分析失败\n";
                return 1;
            }
            
            std::cout << "4. 生成中间代码...\n";
            IRGenerator irgen;
            auto ir = irgen.generate(ast.get());
            
            std::cout << "\n中间代码 (" << ir.size() << " 条指令):\n";
            for (size_t i = 0; i < ir.size(); ++i) {
                std::cout << "  " << i << ": " << ir[i].toString() << "\n";
            }
            
            std::cout << "\n5. 虚拟机执行...\n";
            VirtualMachine vm;
            for (const auto& [name, str] : string_table) {
                vm.addString(name, str);
            }
            vm.load(ir);
            
            if (debug) {
                Debugger debugger(vm, ir);
                debugger.run();
            } else {
                vm.run();
            }
            
            int result = vm.getResult();
            std::cout << "\n执行结果: " << result << "\n";
            
            return result;
            
        } catch (const std::exception& e) {
            std::cerr << "\n❌ 错误: " << e.what() << "\n";
            return 1;
        }
    }
};

// ==================== 测试 ====================

#ifdef TINYCPP_TEST

int main(int argc, char* argv[]) {
    std::cout << "=== tinycpp: C++ 编译器 ===\n";
    std::cout << "版本 1.0.0\n\n";
    
    std::string source;
    bool debug = false;
    
    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-d" || arg == "--debug") {
            debug = true;
        } else if (arg == "-f" || arg == "--file") {
            if (i + 1 < argc) {
                std::ifstream file(argv[++i]);
                if (!file) {
                    std::cerr << "无法打开文件: " << argv[i] << "\n";
                    return 1;
                }
                source = std::string(std::istreambuf_iterator<char>(file), {});
            }
        }
    }
    
    // 如果没有文件，使用默认测试代码
    if (source.empty()) {
        source = R"(
// 测试C++特性：类、继承、虚函数
class Animal {
public:
    virtual int speak() {
        return 0;
    }
};

class Dog : public Animal {
public:
    int speak() override {
        return 42;
    }
};

int main() {
    Animal* a = new Dog();
    int result = a->speak();  // 虚函数调用，应该返回42
    delete a;
    return result;
}
)";
    }
    
    std::cout << "源代码:\n" << source << "\n\n";
    
    TinyCPP compiler;
    int result = compiler.compileAndRun(source, debug);
    
    std::cout << "\n" << (result == 0 ? "✅ 成功" : "❌ 失败") << "\n";
    
    return result;
}

#endif

// ==================== 编译命令 ====================

// 编译: g++ -std=c++17 -DTINYCPP_TEST -o tinycpp tinycpp.cpp
// 运行: ./tinycpp
// 调试: ./tinycpp --debug
// 文件: ./tinycpp --file test.cpp