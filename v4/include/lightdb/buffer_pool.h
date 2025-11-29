#ifndef LIGHTDB_BUFFER_POOL_H
#define LIGHTDB_BUFFER_POOL_H
#include "base.h"
#include "lightdb/page.h"
#include "lightdb/logger.h"
#include <unordered_map>
#include <list> 
#include <mutex>
namespace lightdb {
    struct Frame {
        Page page;
        bool is_dirty;
        int pin_count; 
        std::list<PageID>::iterator lru_iter;
    };
    class BufferPool {
        public:
            explicit BufferPool(int max_frames = 32) :
                max_frames(max_frames) {}
            ~BufferPool() = default;

            Page* FetchPage(PageID page_id);
            void UnpinPage(PageID page_id, bool is_dirty); //release page pin
            void FlushPage(PageID  page_id);
        private:
            void UpdateLRU(PageID page_id);
            void EvictLRU(); //淘汰页尾
            int max_frames;
            std::unordered_map<PageID, Frame> frame_map_;
            std::list<PageID> lru_list_; 
            std::mutex mutex_;
    };
}
#endif 