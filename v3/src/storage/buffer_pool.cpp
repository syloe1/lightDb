#include "lightdb/buffer_pool.h"
#include <iostream>
#include <string>
namespace lightdb {
    Page* BufferPool::FetchPage(PageID page_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = frame_map_.find(page_id);
        if (it != frame_map_.end()) {
            UpdateLRU(it->second.page.page_id);
            it->second.pin_count++;
            LOG_DEBUG("Fetch page " + std::to_string(page_id) + " from buffer");
            return &it->second.page;
        }

        // 页面不在缓冲区，需要加载
        if (frame_map_.size() >= max_frames) {
            EvictLRU();
        }

        // 初始化新帧
        Frame new_frame;
        new_frame.page.page_id = page_id;
        new_frame.is_dirty = false;
        new_frame.pin_count = 1;
        // 插入 LRU 链表头部
        lru_list_.push_front(page_id);
        new_frame.lru_iter = lru_list_.begin(); // 绑定迭代器
        // 存入映射
        frame_map_[page_id] = new_frame;

        LOG_DEBUG("Load page " + std::to_string(page_id) + " from disk");
        return &frame_map_[page_id].page;
    }
    void BufferPool::UnpinPage(PageID page_id, bool is_dirty)  {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = frame_map_.find(page_id);
        if(it == frame_map_.end()) {
            LOG_ERROR("UnpinPage: Page " + std::to_string(page_id) + " not found");
            return;
        }
        if (it->second.pin_count > 0) {
            it->second.pin_count--;
        }
        if(is_dirty) {
            it->second.is_dirty = true;
            it->second.page.is_dirty = true;
        }
        LOG_DEBUG("Unpin page " + std::to_string(page_id) + ", pin_count: " + std::to_string(it->second.pin_count));
    }
    void BufferPool::FlushPage(PageID page_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = frame_map_.find(page_id);
        if(it == frame_map_.end()) {
            LOG_ERROR("FlushPage failed: page " + std::to_string(page_id) + " not found");
            return ;
        }
        if(it->second.is_dirty) {
            LOG_INFO("Flush dirty page " + std::to_string(page_id) + " to disk");
            it->second.is_dirty = false; 
            it->second.page.is_dirty = false;
        } else {
            LOG_DEBUG("Page " + std::to_string(page_id) + " is clean, no need to flush");
        }
    }
    void BufferPool::UpdateLRU(PageID page_id) {
        auto it = frame_map_.find(page_id);
        if (it != frame_map_.end()) {
            // 移除旧位置，插入到头部
            lru_list_.erase(it->second.lru_iter);
            lru_list_.push_front(page_id);
            it->second.lru_iter = lru_list_.begin();
        }
    }

    void BufferPool::EvictLRU() {
        // 找到LRU链表尾部（最久未使用）且pin_count为0的页
        auto it = lru_list_.rbegin();
        while (it != lru_list_.rend()) {
            PageID evict_id = *it;
            auto frame_it = frame_map_.find(evict_id);
            if (frame_it->second.pin_count == 0) {
                // 若为脏页，先刷盘
                if (frame_it->second.is_dirty) {
                    FlushPage(evict_id);
                }
                // 移除帧和LRU节点
                frame_map_.erase(evict_id);
                lru_list_.erase(std::next(it).base());
                LOG_DEBUG("Evict LRU page " + std::to_string(evict_id));
                return;
            }
            it++;
        }
        LOG_ERROR("Evict failed: all pages are pinned");
    }
}