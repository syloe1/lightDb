#ifndef LIGHTDB_PARSER_H
#define LIGHTDB_PARSER_H

#include "lexer.h"
#include "sql_ast.h"
#include <vector>
#include <memory>

namespace lightdb {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}
    
    // 解析入口，返回 Statement 指针
    std::unique_ptr<Statement> ParseSQL();

private:
    const Token& CurrentToken();
    void Advance();
    // 验证并消费 Token，如果类型不匹配则抛出异常
    void Consume(TokenType type, const std::string& error_message);
    
    // 各类语句解析函数
    std::unique_ptr<SelectStatement> ParseSelect();
    std::unique_ptr<InsertStatement> ParseInsert();
    std::unique_ptr<DeleteStatement> ParseDelete();
    std::unique_ptr<UpdateStatement> ParseUpdate();
    std::unique_ptr<CreateStatement> ParseCreate();

    // 辅助解析函数
    std::vector<Condition> ParseWhereClause();
    Value ParseValue();

    std::vector<Token> tokens_;
    size_t pos_;
};

} // namespace lightdb
#endif