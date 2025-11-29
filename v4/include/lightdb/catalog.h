#ifndef LIGHTDB_CATALOG_H
#define LIGHTDB_CATALOG_H

#include "heap_file.h"
#include "bplus_tree.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace lightdb {

struct TableInfo {
    std::string table_name;
    HeapFile* heap_file;
    // 实际项目中还应包含 Schema (列定义)
};

struct IndexInfo {
    std::string index_name;
    std::string table_name;
    std::string column_name;
    BTreeIndex* btree;
};

class Catalog {
public:
    // 注册表
    void RegisterTable(const std::string& table_name, HeapFile* file) {
        tables_[table_name] = {table_name, file};
    }

    // 注册索引
    void RegisterIndex(const std::string& table_name, const std::string& col_name, BTreeIndex* index) {
        std::string key = table_name + "." + col_name;
        indexes_[key] = {"idx_" + key, table_name, col_name, index};
    }

    // 查找表
    bool GetTable(const std::string& table_name) {
        return tables_.find(table_name) != tables_.end();
    }

    // 查找索引 (用于优化器判断)
    IndexInfo* GetIndex(const std::string& table_name, const std::string& col_name) {
        std::string key = table_name + "." + col_name;
        if (indexes_.find(key) != indexes_.end()) {
            return &indexes_[key];
        }
        return nullptr;
    }

private:
    std::unordered_map<std::string, TableInfo> tables_;
    // Key: "table_name.column_name"
    std::unordered_map<std::string, IndexInfo> indexes_; 
};

} // namespace lightdb
#endif