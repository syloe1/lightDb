#include "lightdb/page.h"
namespace lightdb {
    int Page::GetFreeSpace() {
       return PAGE_SIZE - (sizeof(RecordHeader) * record_count + used_data_size);
    }
}