#include "lightdb/bplus_tree.h"
#include "lightdb/logger.h"
#include <cstring>

namespace lightdb {

// 叶子节点序列化
void BTreeLeafNode::Serialize(char* data) const {
    int offset = 0;
    // 节点头信息
    data[offset++] = is_leaf ? 1 : 0;
    memcpy(data + offset, &size, 4); offset += 4;
    memcpy(data + offset, &parent, 4); offset += 4;
    memcpy(data + offset, &prev, 4); offset += 4;
    memcpy(data + offset, &next, 4); offset += 4;
    // 键值对
    for (const auto& entry : entries) {
        memcpy(data + offset, &entry.first, sizeof(KeyType)); offset += sizeof(KeyType);
        memcpy(data + offset, &entry.second.page_id, 4); offset += 4;
        memcpy(data + offset, &entry.second.slot_id, 4); offset += 4;
    }
}

// 叶子节点反序列化
void BTreeLeafNode::Deserialize(const char* data) {
    int offset = 0;
    is_leaf = data[offset++] == 1;
    memcpy(&size, data + offset, 4); offset += 4;
    memcpy(&parent, data + offset, 4); offset += 4;
    memcpy(&prev, data + offset, 4); offset += 4;
    memcpy(&next, data + offset, 4); offset += 4;
    // 读取键值对
    entries.resize(size);
    for (int i = 0; i < size; ++i) {
        memcpy(&entries[i].first, data + offset, sizeof(KeyType)); offset += sizeof(KeyType);
        PageID pid; int slot;
        memcpy(&pid, data + offset, 4); offset += 4;
        memcpy(&slot, data + offset, 4); offset += 4;
        entries[i].second = RID(pid, slot);
    }
}

// 内部节点序列化
void BTreeInternalNode::Serialize(char* data) const {
    int offset = 0;
    // 节点头信息
    data[offset++] = is_leaf ? 1 : 0;
    memcpy(data + offset, &size, 4); offset += 4;
    memcpy(data + offset, &parent, 4); offset += 4;
    // 子节点
    for (PageID child : children) {
        memcpy(data + offset, &child, 4); offset += 4;
    }
    // 关键字
    for (KeyType key : keys) {
        memcpy(data + offset, &key, sizeof(KeyType)); offset += sizeof(KeyType);
    }
}

// 内部节点反序列化
void BTreeInternalNode::Deserialize(const char* data) {
    int offset = 0;
    is_leaf = data[offset++] == 1;
    memcpy(&size, data + offset, 4); offset += 4;
    memcpy(&parent, data + offset, 4); offset += 4;
    // 读取子节点
    children.resize(size + 1);
    for (int i = 0; i < size + 1; ++i) {
        memcpy(&children[i], data + offset, 4); offset += 4;
    }
    // 读取关键字
    keys.resize(size);
    for (int i = 0; i < size; ++i) {
        memcpy(&keys[i], data + offset, sizeof(KeyType)); offset += sizeof(KeyType);
    }
}

// BTreeIndex 辅助函数
std::unique_ptr<BTreeNode> BTreeIndex::FetchNode(PageID pid) {
    if (pid == INVALID_PAGE_ID) return nullptr;
    Page* page = buffer_pool_->FetchPage(pid);
    if (!page) return nullptr;

    const char* data = page->GetData();
    bool is_leaf = data[0] == 1;
    std::unique_ptr<BTreeNode> node;

    if (is_leaf) {
        node = std::make_unique<BTreeLeafNode>(pid);
    } else {
        node = std::make_unique<BTreeInternalNode>(pid);
    }
    node->Deserialize(data);
    buffer_pool_->UnpinPage(pid, false);
    return node;
}

void BTreeIndex::SaveNode(const BTreeNode* node) {
    Page* page = buffer_pool_->FetchPage(node->page_id);
    if (!page) {
        LOG_ERROR("Failed to save node: page " + std::to_string(node->page_id) + " not found");
        return;
    }
    node->Serialize(page->GetData());
    buffer_pool_->UnpinPage(node->page_id, true); // 标记脏页
}

PageID BTreeIndex::AllocatePage() {
    return next_page_id_++;
}

// 插入实现
bool BTreeIndex::Insert(const KeyType& key, const ValueType& value) {
    KeyType split_key;
    PageID new_page_id = INVALID_PAGE_ID;

    if (InsertHelper(root_page_id_, key, value, split_key, new_page_id)) {
        // 根节点分裂需要创建新根
        if (new_page_id != INVALID_PAGE_ID) {
            PageID new_root_id = AllocatePage();
            auto new_root = std::make_unique<BTreeInternalNode>(new_root_id);
            new_root->children.push_back(root_page_id_);
            new_root->children.push_back(new_page_id);
            new_root->keys.push_back(split_key);
            new_root->size = 1;

            // 更新旧根的父节点
            auto old_root = FetchNode(root_page_id_);
            old_root->parent = new_root_id;
            SaveNode(old_root.get());

            // 更新新分裂节点的父节点
            auto new_node = FetchNode(new_page_id);
            new_node->parent = new_root_id;
            SaveNode(new_node.get());

            // 保存新根
            SaveNode(new_root.get());
            root_page_id_ = new_root_id;
        }
        return true;
    }
    return false;
}

bool BTreeIndex::InsertHelper(PageID pid, const KeyType& key, const ValueType& value, KeyType& split_key, PageID& new_page_id) {
    auto node = FetchNode(pid);
    if (!node) return false;

    if (node->is_leaf) {
        auto* leaf = static_cast<BTreeLeafNode*>(node.get());
        // 查找插入位置
        auto it = std::lower_bound(leaf->entries.begin(), leaf->entries.end(), key,
            [](const auto& entry, const KeyType& k) { return entry.first < k; });

        // 检查重复键
        if (it != leaf->entries.end() && it->first == key) {
            LOG_WARN("Duplicate key insertion: " + std::to_string(key));
            return false;
        }

        // 插入新键值对
        leaf->entries.insert(it, {key, value});
        leaf->size++;
        SaveNode(leaf);

        // 检查是否需要分裂
        if (leaf->size >= order_ - 1) {
            PageID new_leaf_id = AllocatePage();
            auto new_leaf = std::make_unique<BTreeLeafNode>(new_leaf_id);
            new_leaf->parent = leaf->parent;

            // 分裂逻辑
            SplitLeafNode(leaf, new_leaf.get(), split_key);

            // 更新链表指针
            new_leaf->next = leaf->next;
            if (new_leaf->next != INVALID_PAGE_ID) {
                auto next_leaf = FetchNode(new_leaf->next);
                static_cast<BTreeLeafNode*>(next_leaf.get())->prev = new_leaf_id;
                SaveNode(next_leaf.get());
            }
            leaf->next = new_leaf_id;
            new_leaf->prev = leaf->page_id;

            SaveNode(leaf);
            SaveNode(new_leaf.get());
            new_page_id = new_leaf_id;
            return true;
        }
        new_page_id = INVALID_PAGE_ID;
        return true;
    } else {
        auto* internal = static_cast<BTreeInternalNode*>(node.get());
        // 查找子节点位置
        int pos = 0;
        while (pos < internal->size && key > internal->keys[pos]) {
            pos++;
        }

        // 递归插入子节点
        KeyType child_split_key;
        PageID child_new_page_id = INVALID_PAGE_ID;
        if (!InsertHelper(internal->children[pos], key, value, child_split_key, child_new_page_id)) {
            return false;
        }

        // 子节点分裂，需要插入中间键
        if (child_new_page_id != INVALID_PAGE_ID) {
            internal->keys.insert(internal->keys.begin() + pos, child_split_key);
            internal->children.insert(internal->children.begin() + pos + 1, child_new_page_id);
            internal->size++;
            SaveNode(internal);

            // 检查当前节点是否需要分裂
            if (internal->size >= order_ - 1) {
                PageID new_internal_id = AllocatePage();
                auto new_internal = std::make_unique<BTreeInternalNode>(new_internal_id);
                new_internal->parent = internal->parent;

                SplitInternalNode(internal, new_internal.get(), split_key);

                // 更新子节点的父指针
                for (PageID child : new_internal->children) {
                    auto child_node = FetchNode(child);
                    child_node->parent = new_internal_id;
                    SaveNode(child_node.get());
                }

                SaveNode(internal);
                SaveNode(new_internal.get());
                new_page_id = new_internal_id;
                return true;
            }
        }

        new_page_id = INVALID_PAGE_ID;
        return true;
    }
}

void BTreeIndex::SplitLeafNode(const BTreeLeafNode* leaf, BTreeLeafNode* new_leaf, KeyType& split_key) {
    int mid = leaf->size / 2;
    split_key = leaf->entries[mid].first;

    // 移动后半部分数据到新节点
    new_leaf->entries = std::vector<std::pair<KeyType, ValueType>>(
        leaf->entries.begin() + mid, leaf->entries.end()
    );
    new_leaf->size = leaf->entries.size() - mid;

    // 保留前半部分数据
    const_cast<BTreeLeafNode*>(leaf)->entries.erase(
        leaf->entries.begin() + mid, leaf->entries.end()
    );
    const_cast<BTreeLeafNode*>(leaf)->size = mid;
}

void BTreeIndex::SplitInternalNode(const BTreeInternalNode* internal, BTreeInternalNode* new_internal, KeyType& split_key) {
    int mid = internal->size / 2;
    split_key = internal->keys[mid];

    // 移动后半部分关键字和子节点
    new_internal->keys = std::vector<KeyType>(
        internal->keys.begin() + mid + 1, internal->keys.end()
    );
    new_internal->children = std::vector<PageID>(
        internal->children.begin() + mid + 1, internal->children.end()
    );
    new_internal->size = internal->size - (mid + 1);

    // 保留前半部分
    const_cast<BTreeInternalNode*>(internal)->keys.erase(
        internal->keys.begin() + mid, internal->keys.end()
    );
    const_cast<BTreeInternalNode*>(internal)->children.erase(
        internal->children.begin() + mid + 1, internal->children.end()
    );
    const_cast<BTreeInternalNode*>(internal)->size = mid;
}

// 查找实现
bool BTreeIndex::Search(const KeyType& key, ValueType& value) {
    PageID current = root_page_id_;
    while (current != INVALID_PAGE_ID) {
        auto node = FetchNode(current);
        if (!node) return false;

        if (node->is_leaf) {
            auto* leaf = static_cast<BTreeLeafNode*>(node.get());
            auto it = std::lower_bound(leaf->entries.begin(), leaf->entries.end(), key,
                [](const auto& entry, const KeyType& k) { return entry.first < k; });
            if (it != leaf->entries.end() && it->first == key) {
                value = it->second;
                return true;
            }
            return false;
        } else {
            auto* internal = static_cast<BTreeInternalNode*>(node.get());
            int pos = 0;
            while (pos < internal->size && key > internal->keys[pos]) {
                pos++;
            }
            current = internal->children[pos];
        }
    }
    return false;
}

// 范围扫描实现
PageID BTreeIndex::FindFirstLeaf(const KeyType& key) {
    PageID current = root_page_id_;
    while (current != INVALID_PAGE_ID) {
        auto node = FetchNode(current);
        if (!node) return INVALID_PAGE_ID;

        if (node->is_leaf) {
            return current;
        } else {
            auto* internal = static_cast<BTreeInternalNode*>(node.get());
            int pos = 0;
            while (pos < internal->size && key > internal->keys[pos]) {
                pos++;
            }
            current = internal->children[pos];
        }
    }
    return INVALID_PAGE_ID;
}

std::vector<ValueType> BTreeIndex::RangeScan(const KeyType& start, const KeyType& end) {
    std::vector<ValueType> result;
    PageID current_pid = FindFirstLeaf(start);

    while (current_pid != INVALID_PAGE_ID) {
        auto node = FetchNode(current_pid);
        if (!node || !node->is_leaf) break;

        auto* leaf = static_cast<BTreeLeafNode*>(node.get());
        // 收集当前叶子中符合条件的记录
        for (const auto& entry : leaf->entries) {
            if (entry.first > end) break;
            if (entry.first >= start) {
                result.push_back(entry.second);
            }
        }

        current_pid = leaf->next;
    }
    return result;
}

// 删除实现（简化版，仅处理基础删除逻辑，未实现合并）
bool BTreeIndex::Delete(const KeyType& key) {
    return DeleteHelper(root_page_id_, key);
}

bool BTreeIndex::DeleteHelper(PageID pid, const KeyType& key) {
    auto node = FetchNode(pid);
    if (!node) return false;

    if (node->is_leaf) {
        auto* leaf = static_cast<BTreeLeafNode*>(node.get());
        auto it = std::lower_bound(leaf->entries.begin(), leaf->entries.end(), key,
            [](const auto& entry, const KeyType& k) { return entry.first < k; });

        if (it != leaf->entries.end() && it->first == key) {
            leaf->entries.erase(it);
            leaf->size--;
            SaveNode(leaf);
            return true;
        }
        return false;
    } else {
        auto* internal = static_cast<BTreeInternalNode*>(node.get());
        int pos = 0;
        while (pos < internal->size && key > internal->keys[pos]) {
            pos++;
        }

        if (DeleteHelper(internal->children[pos], key)) {
            // 此处可添加节点合并逻辑（根据阶数阈值判断）
            return true;
        }
    }
    return false;
}

} // namespace lightdb