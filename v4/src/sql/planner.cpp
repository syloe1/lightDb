#include "lightdb/planner.h"
#include "lightdb/logger.h"

namespace lightdb {

std::unique_ptr<Plan> Planner::PlanQuery(Statement* stmt) {
    if (!stmt) return nullptr;

    switch (stmt->type) {
        case StatementType::SELECT:
            return PlanSelect(static_cast<SelectStatement*>(stmt));
        case StatementType::INSERT:
            return PlanInsert(static_cast<InsertStatement*>(stmt));
        case StatementType::CREATE_TABLE:
            return PlanCreateTable(static_cast<CreateStatement*>(stmt));
        default:
            LOG_ERROR("Unsupported statement type in planner");
            return nullptr;
    }
}

// 简单的基于规则的优化器
// 规则：如果在 WHERE 子句中发现了有索引的列，优先生成 IndexScan，否则生成 SeqScan
std::unique_ptr<Plan> Planner::PlanSelect(SelectStatement* stmt) {
    std::string table_name = stmt->table_name;
    
    // 1. 检查是否有 WHERE 条件
    if (!stmt->where_clauses.empty()) {
        // 遍历所有条件，寻找可以用索引的条件
        for (const auto& cond : stmt->where_clauses) {
            // 询问 Catalog：该表的该列是否有索引？
            IndexInfo* idx = catalog_->GetIndex(table_name, cond.column);
            
            if (idx != nullptr) {
                // [优化器命中] 发现索引！生成 IndexScanPlan
                LOG_INFO("Optimizer: Found index on " + table_name + "." + cond.column + ", using IndexScan.");
                return std::make_unique<IndexScanPlan>(
                    table_name, 
                    idx->index_name, 
                    cond.column, 
                    cond.op, 
                    cond.value
                );
            }
        }
    }

    // 2. [默认回退] 没有索引或没有 WHERE 条件，生成 SeqScanPlan
    LOG_INFO("Optimizer: No suitable index found, using SeqScan.");
    return std::make_unique<SeqScanPlan>(table_name, stmt->where_clauses);
}

std::unique_ptr<Plan> Planner::PlanInsert(InsertStatement* stmt) {
    return std::make_unique<InsertPlan>(stmt->table_name, stmt->values);
}

std::unique_ptr<Plan> Planner::PlanCreateTable(CreateStatement* stmt) {
    return std::make_unique<CreateTablePlan>(stmt->table_name, stmt->columns);
}

} // namespace lightdb