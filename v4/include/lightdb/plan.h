#ifndef LIGHTDB_PLAN_H
#define LIGHTDB_PLAN_H

#include "sql_ast.h"
#include "base.h"
#include <string>
#include <vector>
#include <memory>

namespace lightdb {

enum class PlanType {
    SEQ_SCAN,
    INDEX_SCAN,
    INSERT,
    DELETE,
    UPDATE,
    CREATE_TABLE
};

// 计划基类
struct Plan {
    PlanType type;
    virtual ~Plan() = default;
};

// 1. 全表扫描计划 (无索引时)
struct SeqScanPlan : Plan {
    std::string table_name;
    std::vector<Condition> predicates; // 过滤条件
    
    SeqScanPlan(std::string table, std::vector<Condition> preds) 
        : table_name(std::move(table)), predicates(std::move(preds)) {
        type = PlanType::SEQ_SCAN;
    }
};

// 2. 索引扫描计划 (有索引时)
// 简化起见，只支持单键查询
struct IndexScanPlan : Plan {
    std::string table_name;
    std::string index_name; // 比如 "users_id_idx"
    std::string column_name; // 索引列名
    std::string op;          // =, <, >
    Value value;             // 查找的值
    
    IndexScanPlan(std::string table, std::string idx_name, std::string col, std::string op_str, Value val)
        : table_name(std::move(table)), index_name(std::move(idx_name)), 
          column_name(std::move(col)), op(std::move(op_str)), value(std::move(val)) {
        type = PlanType::INDEX_SCAN;
    }
};

// 3. 插入计划
struct InsertPlan : Plan {
    std::string table_name;
    std::vector<Value> values;
    
    InsertPlan(std::string table, std::vector<Value> vals)
        : table_name(std::move(table)), values(std::move(vals)) {
        type = PlanType::INSERT;
    }
};

// 4. 创建表计划
struct CreateTablePlan : Plan {
    std::string table_name;
    std::vector<ColumnDef> columns;
    CreateTablePlan(std::string table, std::vector<ColumnDef> cols)
        : table_name(std::move(table)), columns(std::move(cols)) {
        type = PlanType::CREATE_TABLE;
    }
};

// ... Update/Delete Plan 可按需扩展

} // namespace lightdb

#endif