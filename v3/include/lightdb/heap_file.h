#ifndef LIGHTDB_HEAP_FILE_H
#define LIGHTDB_HEAP_FILE_H

#include "lightdb/page.h"
#include "lightdb/buffer_pool.h"
#include <string>
#include <vector>
namespace lightdb {
    struct Record {
        std::string data;
        RID rid;
    };
    class HeapFile {
        public:
            explicit HeapFile(const std::string& file_path, BufferPool* buffer_pool)
                : file_path_(file_path), buffer_pool_(buffer_pool), next_page_id_(0){}
            RID InsertRecord(const Record& record);
            Record ReadRecord(const RID& rid);
            bool DeleteRecord(const RID& rid);
            std::vector<Record> SeqScan(); //扫描所有未删除记录
        private:
            Page* GetFreePage(const Record& record);
            int SerializeRecord(const Record& record, char* dest);

            std::string file_path_;
            BufferPool* buffer_pool_;
            PageID next_page_id_;
    };
}
#endif 