//src/main.cpp
#include "lightdb/logger.h"
#include "lightdb/heap_file.h"
#include "lightdb/buffer_pool.h"
#include "lightdb/base.h"
#include "lightdb/bplus_tree.h"  // 新增引用
#include <cassert>  // 用于断言测试

int main() {
    // 初始化日志
    lightdb::Logger::GetInstance().SetLogLevel(lightdb::LogLevel::DEBUG);
    LOG_INFO("LightDB 基础框架启动成功！");
    LOG_DEBUG("当前 C++ 标准：C++17");

    // 测试基础类型
    lightdb::RID rid(1, 0);
    LOG_INFO("测试 RID：" + rid.ToString());

    lightdb::Tuple tuple;
    tuple.AddField("1");
    tuple.AddField("Alice");
    LOG_INFO("测试 Tuple：id=" + tuple.GetField(0) + ", name=" + tuple.GetField(1));

    LOG_INFO("Stage 1 仓库初始化与基础框架搭建完成！");

    // 测试HeapFile
    lightdb::BufferPool buffer_pool(1024);  // 增大缓冲池
    lightdb::HeapFile heap_file("test_table.db", &buffer_pool);
    for(int i = 0; i < 100; i++) {
        lightdb::Record record;
        record.data = "user_" + std::to_string(i) + ", page_" + std::to_string(20 + i % 10);
        heap_file.InsertRecord(record);
    }
    LOG_INFO("Insert 100 records to HeapFile successfully");
    std::vector<lightdb::Record> records = heap_file.SeqScan();
    LOG_INFO("SeqScan result: total " + std::to_string(records.size()) + " records");

    // 新增B+Tree测试
    lightdb::BTreeIndex btree(&buffer_pool, 200);  // 阶数200
    std::vector<lightdb::RID> test_rids;

    // 插入1万条数据
    for (int i = 0; i < 10000; ++i) {
        lightdb::RID test_rid(0, i);  // 模拟HeapFile中的RID
        btree.Insert(i, test_rid);
        test_rids.push_back(test_rid);
    }
    LOG_INFO("Insert 10000 records to B+Tree successfully");

    // 测试查找
    lightdb::RID found_rid;
    assert(btree.Search(5000, found_rid) && found_rid == test_rids[5000]);
    LOG_INFO("B+Tree Search test passed");

    // 测试范围扫描
    auto range_result = btree.RangeScan(1000, 2000);
    assert(range_result.size() == 1001);  // 包含1000到2000共1001个值
    LOG_INFO("B+Tree RangeScan test passed");

    // 测试删除
    assert(btree.Delete(5000));
    assert(!btree.Search(5000, found_rid));
    LOG_INFO("B+Tree Delete test passed");

    // 扩展测试10万数据
    for (int i = 10000; i < 100000; ++i) {
        lightdb::RID test_rid(0, i);
        btree.Insert(i, test_rid);
    }
    assert(btree.Search(99999, found_rid) && found_rid.slot_id == 99999);
    LOG_INFO("B+Tree 100000 records test passed");

    LOG_INFO("Storage Engine Stage Test completed");
    return 0;
}