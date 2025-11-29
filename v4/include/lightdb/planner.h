#ifndef LIGHTDB_PLANNER_H
#define LIGHTDB_PLANNER_H

#include "sql_ast.h"
#include "plan.h"
#include "catalog.h"
#include <memory>

namespace lightdb {

class Planner {
public:
    explicit Planner(Catalog* catalog) : catalog_(catalog) {}

    // 核心入口：AST -> Physical Plan
    std::unique_ptr<Plan> PlanQuery(Statement* stmt);

private:
    std::unique_ptr<Plan> PlanSelect(SelectStatement* stmt);
    std::unique_ptr<Plan> PlanInsert(InsertStatement* stmt);
    std::unique_ptr<Plan> PlanCreateTable(CreateStatement* stmt);
    // ... delete, update

    Catalog* catalog_;
};

} // namespace lightdb
#endif