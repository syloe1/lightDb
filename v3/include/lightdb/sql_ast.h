#ifndef LIGHTDB_SQL_AST_H
#define LIGHTDB_SQL_AST_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace lightdb {

// --- 词法分析相关 ---

enum class TokenType {
    // Keywords
    SELECT, INSERT, DELETE, UPDATE, CREATE, TABLE, FROM, WHERE, INTO, VALUES, SET, AND, OR,
    INT_TYPE, VARCHAR_TYPE,
    // Symbols
    ASTERISK, COMMA, SEMICOLON, LPAREN, RPAREN, EQ, NEQ, GT, LT, GTE, LTE, ASSIGN,
    // Literals & Identifiers
    IDENTIFIER, INT_LITERAL, STRING_LITERAL,
    END_OF_INPUT
};

struct Token {
    TokenType type;
    std::string value;
    
    std::string ToString() const {
        return "Token(" + value + ")";
    }
};

// --- AST 相关 ---

enum class StatementType {
    CREATE_TABLE,
    SELECT,
    INSERT,
    DELETE,
    UPDATE
};

// 简单的值表达式
struct Value {
    enum Type { INT, STRING, COLUMN_REF } type;
    std::string value; 
};

// WHERE 条件：简单的二元表达式 (A op B)
struct Condition {
    std::string column;
    std::string op; // =, >, <, etc.
    Value value;
};

// 列定义 (用于 Create Table)
struct ColumnDef {
    std::string name;
    std::string type; // "int", "varchar"
    int length; // for varchar
};

// 基类
struct Statement {
    StatementType type;
    virtual ~Statement() = default;
};

// CREATE TABLE name (col1 type, col2 type)
struct CreateStatement : Statement {
    std::string table_name;
    std::vector<ColumnDef> columns;
    CreateStatement() { type = StatementType::CREATE_TABLE; }
};

// SELECT * FROM table WHERE ...
struct SelectStatement : Statement {
    std::string table_name;
    std::vector<std::string> columns; // "*" means all
    std::vector<Condition> where_clauses;
    SelectStatement() { type = StatementType::SELECT; }
};

// INSERT INTO table VALUES (v1, v2)
struct InsertStatement : Statement {
    std::string table_name;
    std::vector<Value> values;
    InsertStatement() { type = StatementType::INSERT; }
};

// DELETE FROM table WHERE ...
struct DeleteStatement : Statement {
    std::string table_name;
    std::vector<Condition> where_clauses;
    DeleteStatement() { type = StatementType::DELETE; }
};

// UPDATE table SET col=val WHERE ...
struct UpdateStatement : Statement {
    std::string table_name;
    std::vector<std::pair<std::string, Value>> updates; // col = val
    std::vector<Condition> where_clauses;
    UpdateStatement() { type = StatementType::UPDATE; }
};

} // namespace lightdb

#endif