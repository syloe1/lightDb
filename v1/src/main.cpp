//src/main.cpp
#include "lightdb/logger.h"
#include "lightdb/heap_file.h"
#include "lightdb/buffer_pool.h"
#include "lightdb/base.h"
int main() {
    // 初始化日志，输出测试信息
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

    lightdb::BufferPool buffer_pool;
    lightdb::HeapFile heap_file("test_table.db", &buffer_pool);
    for(int i = 0; i < 100; i++) {
        lightdb::Record record;
        record.data = "user_" + std::to_string(i) + ", page_" + std::to_string(20 + i % 10);
        heap_file.InsertRecord(record);
    }
    LOG_INFO("Insert 100 records successfully");
    std::vector<lightdb::Record> records = heap_file.SeqScan();
    LOG_INFO("SeqScan result: total " + std::to_string(records.size()) + " records");
    for(const auto& rec : records) {
        LOG_DEBUG("Record data: " + rec.data + ", RID: " + rec.rid.ToString());
    }
    LOG_INFO("Stotage Engine Stage Test completed");
    return 0;
}