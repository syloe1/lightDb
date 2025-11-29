#include "lightdb/parser.h"
#include "lightdb/logger.h"
#include <stdexcept>

namespace lightdb {

const Token& Parser::CurrentToken() {
    if (pos_ >= tokens_.size()) {
        static Token end_token{TokenType::END_OF_INPUT, ""};
        return end_token;
    }
    return tokens_[pos_];
}

void Parser::Advance() {
    if (pos_ < tokens_.size()) pos_++;
}

void Parser::Consume(TokenType type, const std::string& msg) {
    if (CurrentToken().type == type) {
        Advance();
    } else {
        throw std::runtime_error("Syntax Error: " + msg + ". Got: " + CurrentToken().value);
    }
}

std::unique_ptr<Statement> Parser::ParseSQL() {
    if (tokens_.empty()) return nullptr;

    const auto& token = CurrentToken();
    switch (token.type) {
        case TokenType::SELECT: return ParseSelect();
        case TokenType::INSERT: return ParseInsert();
        case TokenType::DELETE: return ParseDelete();
        case TokenType::UPDATE: return ParseUpdate();
        case TokenType::CREATE: return ParseCreate();
        case TokenType::END_OF_INPUT: return nullptr;
        default:
            throw std::runtime_error("Unknown statement type: " + token.value);
    }
}

// 1. SELECT col1, col2 FROM table WHERE ...
std::unique_ptr<SelectStatement> Parser::ParseSelect() {
    auto stmt = std::make_unique<SelectStatement>();
    Consume(TokenType::SELECT, "Expected SELECT");

    // Columns
    if (CurrentToken().type == TokenType::ASTERISK) {
        stmt->columns.push_back("*");
        Advance();
    } else {
        while (true) {
            stmt->columns.push_back(CurrentToken().value);
            Consume(TokenType::IDENTIFIER, "Expected column name");
            if (CurrentToken().type == TokenType::COMMA) {
                Advance();
            } else {
                break;
            }
        }
    }

    Consume(TokenType::FROM, "Expected FROM");
    stmt->table_name = CurrentToken().value;
    Consume(TokenType::IDENTIFIER, "Expected table name");

    if (CurrentToken().type == TokenType::WHERE) {
        stmt->where_clauses = ParseWhereClause();
    }

    Consume(TokenType::SEMICOLON, "Expected ;");
    return stmt;
}

// 2. INSERT INTO table VALUES (v1, v2)
std::unique_ptr<InsertStatement> Parser::ParseInsert() {
    auto stmt = std::make_unique<InsertStatement>();
    Consume(TokenType::INSERT, "Expected INSERT");
    Consume(TokenType::INTO, "Expected INTO");
    
    stmt->table_name = CurrentToken().value;
    Consume(TokenType::IDENTIFIER, "Expected table name");
    
    Consume(TokenType::VALUES, "Expected VALUES");
    Consume(TokenType::LPAREN, "Expected (");

    while (true) {
        stmt->values.push_back(ParseValue());
        if (CurrentToken().type == TokenType::COMMA) {
            Advance();
        } else {
            break;
        }
    }
    
    Consume(TokenType::RPAREN, "Expected )");
    Consume(TokenType::SEMICOLON, "Expected ;");
    return stmt;
}

// 3. CREATE TABLE table (col type, ...)
std::unique_ptr<CreateStatement> Parser::ParseCreate() {
    auto stmt = std::make_unique<CreateStatement>();
    Consume(TokenType::CREATE, "Expected CREATE");
    Consume(TokenType::TABLE, "Expected TABLE");
    
    stmt->table_name = CurrentToken().value;
    Consume(TokenType::IDENTIFIER, "Expected table name");
    
    Consume(TokenType::LPAREN, "Expected (");

    while (true) {
        ColumnDef col;
        col.name = CurrentToken().value;
        Consume(TokenType::IDENTIFIER, "Expected column name");
        
        if (CurrentToken().type == TokenType::INT_TYPE) {
            col.type = "int";
            col.length = 4;
            Advance();
        } else if (CurrentToken().type == TokenType::VARCHAR_TYPE) {
            col.type = "varchar";
            Advance();
            // Optional length handling: VARCHAR(255)
            if (CurrentToken().type == TokenType::LPAREN) {
                Advance();
                col.length = std::stoi(CurrentToken().value);
                Consume(TokenType::INT_LITERAL, "Expected length");
                Consume(TokenType::RPAREN, "Expected )");
            } else {
                col.length = 255; // Default
            }
        } else {
            throw std::runtime_error("Unsupported column type");
        }

        stmt->columns.push_back(col);

        if (CurrentToken().type == TokenType::COMMA) {
            Advance();
        } else {
            break;
        }
    }

    Consume(TokenType::RPAREN, "Expected )");
    Consume(TokenType::SEMICOLON, "Expected ;");
    return stmt;
}

// 4. DELETE FROM table WHERE ...
std::unique_ptr<DeleteStatement> Parser::ParseDelete() {
    auto stmt = std::make_unique<DeleteStatement>();
    Consume(TokenType::DELETE, "Expected DELETE");
    Consume(TokenType::FROM, "Expected FROM");
    
    stmt->table_name = CurrentToken().value;
    Consume(TokenType::IDENTIFIER, "Expected table name");

    if (CurrentToken().type == TokenType::WHERE) {
        stmt->where_clauses = ParseWhereClause();
    }
    
    Consume(TokenType::SEMICOLON, "Expected ;");
    return stmt;
}

// 5. UPDATE table SET col=val WHERE ...
std::unique_ptr<UpdateStatement> Parser::ParseUpdate() {
    auto stmt = std::make_unique<UpdateStatement>();
    Consume(TokenType::UPDATE, "Expected UPDATE");
    
    stmt->table_name = CurrentToken().value;
    Consume(TokenType::IDENTIFIER, "Expected table name");
    
    Consume(TokenType::SET, "Expected SET");
    
    while(true) {
        std::string col = CurrentToken().value;
        Consume(TokenType::IDENTIFIER, "Expected column");
        Consume(TokenType::EQ, "Expected =");
        Value val = ParseValue();
        stmt->updates.push_back({col, val});
        
        if (CurrentToken().type == TokenType::COMMA) {
            Advance();
        } else {
            break;
        }
    }

    if (CurrentToken().type == TokenType::WHERE) {
        stmt->where_clauses = ParseWhereClause();
    }

    Consume(TokenType::SEMICOLON, "Expected ;");
    return stmt;
}

// Helpers
Value Parser::ParseValue() {
    Value val;
    if (CurrentToken().type == TokenType::INT_LITERAL) {
        val.type = Value::INT;
        val.value = CurrentToken().value;
        Advance();
    } else if (CurrentToken().type == TokenType::STRING_LITERAL) {
        val.type = Value::STRING;
        val.value = CurrentToken().value;
        Advance();
    } else {
         throw std::runtime_error("Expected value");
    }
    return val;
}

std::vector<Condition> Parser::ParseWhereClause() {
    std::vector<Condition> conditions;
    Consume(TokenType::WHERE, "Expected WHERE");

    // Simplified: supports "col op val AND col op val"
    while (true) {
        Condition cond;
        cond.column = CurrentToken().value;
        Consume(TokenType::IDENTIFIER, "Expected column");
        
        cond.op = CurrentToken().value;
        // Simple validation for operators
        if (CurrentToken().type != TokenType::EQ && CurrentToken().type != TokenType::GT && 
            CurrentToken().type != TokenType::LT) {
            // Add other ops...
             Advance(); // Assume valid op for now
        } else {
             Advance();
        }

        cond.value = ParseValue();
        conditions.push_back(cond);

        if (CurrentToken().type == TokenType::AND) {
            Advance();
        } else {
            break;
        }
    }
    return conditions;
}

} // namespace lightdb