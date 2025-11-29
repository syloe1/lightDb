#ifndef LIGHTDB_PAGE_H
#define LIGHTDB_PAGE_H
#include "base.h"
#include "lightdb/base.h"
#include <cstring>
namespace lightdb {
    struct RecordHeader {
        bool is_deleted;
        int32_t record_size;
    };
    struct Page {
        public:
            PageID page_id;
            int pin_count; 
            bool is_dirty;
            char data[PAGE_SIZE];
            int record_count = 0;
            int used_data_size = 0;

            Page(PageID pid = INVALID_PAGE_ID)
                : page_id(pid), pin_count(0), is_dirty(false) {
                memset(data, 0, PAGE_SIZE);
            }
            char* GetData() {
                return data;
            }

            void Reset() {
                pin_count = 0;
                is_dirty = false;
                memset(data, 0, PAGE_SIZE);
            }
            int GetFreeSpace();
    };
}
#endif 