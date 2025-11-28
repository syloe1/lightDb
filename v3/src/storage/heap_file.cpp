#include "lightdb/heap_file.h"
#include <cstring>
namespace lightdb {
    RID HeapFile::InsertRecord(const Record& record) {
         Page* page = GetFreePage(record);
        if (page == nullptr) {
            LOG_ERROR("InsertRecord failed: no free page available");
            return RID();
        }

        int required_space = sizeof(RecordHeader) + record.data.size();
        if (page->GetFreeSpace() < required_space) {
            LOG_ERROR("Page " + std::to_string(page->page_id) + " has no enough space");
            buffer_pool_->UnpinPage(page->page_id, false);
            return RID();
        }

        // 序列化记录到页面
        char* data = page->GetData() + (PAGE_SIZE - page->GetFreeSpace()); // 从空闲空间起始位置写入
        int record_len = SerializeRecord(record, data);

        // 更新页面元数据
        page->record_count++;
        page->used_data_size += record.data.size();  // 需在Page中新增used_data_size字段

        RID rid(page->page_id, page->record_count - 1); // slot_id为记录索引
        LOG_INFO("Insert record to RID: " + rid.ToString());

        buffer_pool_->UnpinPage(page->page_id, true); // 标记脏页
        return rid;
    }
    Record HeapFile::ReadRecord(const RID& rid) {
        Page* page = buffer_pool_->FetchPage(rid.page_id);
        if (page == nullptr) {
            LOG_ERROR("ReadRecord failed: page " + std::to_string(rid.page_id) + " not found");
            return Record();
        }

        char* current = page->GetData();
        RecordHeader header;
        Record record;

        // 遍历到目标 slot_id 对应的记录
        for (int i = 0; i <= rid.slot_id; ++i) {
            // 读取当前记录的头部
            memcpy(&header, current, sizeof(RecordHeader));
            // 检查是否越界（slot_id 超出实际记录数）
            if (i > page->record_count - 1) {
                LOG_ERROR("Invalid slot_id: " + std::to_string(rid.slot_id));
                break;
            }
            // 找到目标记录
            if (i == rid.slot_id) {
                if (!header.is_deleted) {
                    record.data = std::string(current + sizeof(RecordHeader), header.record_size);
                    record.rid = rid;
                }
                break;
            }
            // 移动到下一个记录
            current += sizeof(RecordHeader) + header.record_size;
        }

        buffer_pool_->UnpinPage(page->page_id, false);
        return record;
    }
    bool HeapFile::DeleteRecord(const RID& rid) {
        Page* page = buffer_pool_->FetchPage(rid.page_id);
        if (page == nullptr) {
            LOG_ERROR("DeleteRecord failed: page not found");
            return false;
        }

        char* current = page->GetData();
        RecordHeader header;

        // 遍历到目标 slot_id 对应的记录
        for (int i = 0; i <= rid.slot_id; ++i) {
            memcpy(&header, current, sizeof(RecordHeader));
            if (i > page->record_count - 1) {
                LOG_ERROR("Invalid slot_id: " + std::to_string(rid.slot_id));
                buffer_pool_->UnpinPage(page->page_id, false);
                return false;
            }
            if (i == rid.slot_id) {
                header.is_deleted = true;
                memcpy(current, &header, sizeof(RecordHeader)); // 更新头部
                buffer_pool_->UnpinPage(page->page_id, true); // 标记脏页
                LOG_INFO("Delete record at RID: " + rid.ToString());
                return true;
            }
            current += sizeof(RecordHeader) + header.record_size;
        }

        buffer_pool_->UnpinPage(page->page_id, false);
        return false;
    }

    std::vector<Record> HeapFile::SeqScan() {
        std::vector<Record> records;
        PageID current_page_id = 0;

        while (current_page_id < next_page_id_) {
            Page* page = buffer_pool_->FetchPage(current_page_id);
            if (page == nullptr) {
                current_page_id++;
                continue;
            }

            char* current = page->GetData();
            // 遍历页内所有记录
            for (int i = 0; i < page->record_count; ++i) {
                RecordHeader header;
                memcpy(&header, current, sizeof(RecordHeader));
                if (!header.is_deleted) {
                    Record record;
                    record.data = std::string(current + sizeof(RecordHeader), header.record_size);
                    record.rid = RID(current_page_id, i);
                    records.push_back(record);
                }
                // 移动到下一个记录
                current += sizeof(RecordHeader) + header.record_size;
            }

            buffer_pool_->UnpinPage(current_page_id, false);
            current_page_id++;
        }

        LOG_INFO("SeqScan completed, total records: " + std::to_string(records.size()));
        return records;
    }

    Page* HeapFile::GetFreePage(const Record& record) {
        // 简化实现：直接创建新页，实际需遍历查找已有空闲页
         for (PageID pid = 0; pid < next_page_id_; pid++) {
            Page* page = buffer_pool_->FetchPage(pid);
            if (page == nullptr) continue;

            // 检查页面空闲空间是否足够存储新记录（假设记录大小已知）
            int required_space = sizeof(RecordHeader) + record.data.size(); // 需传入记录大小
            if (page->GetFreeSpace() >= required_space) {
                buffer_pool_->UnpinPage(pid, false); // 暂时不修改，仅检查
                return page;
            }
            buffer_pool_->UnpinPage(pid, false);
        }

        // 2. 若没有可用页面，再创建新页
        Page* new_page = buffer_pool_->FetchPage(next_page_id_);
        next_page_id_++;
        return new_page;
    }

    int HeapFile::SerializeRecord(const Record& record, char* dest) {
        // 序列化格式：RecordHeader + 记录数据
        RecordHeader header{false, static_cast<int32_t>(record.data.size())};
        memcpy(dest, &header, sizeof(RecordHeader));
        memcpy(dest + sizeof(RecordHeader), record.data.c_str(), record.data.size());
        return sizeof(RecordHeader) + record.data.size();
    }

}