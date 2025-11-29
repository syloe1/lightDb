#ifndef LIGHTDB_BPLUS_TREE_H
#define LIGHTDB_BPLUS_TREE_H

#include "base.h"
#include "buffer_pool.h"
#include "page.h"
#include <vector>
#include <memory>
#include <algorithm>

namespace lightdb {

using KeyType = int;  // 可扩展为支持字符串等类型
using ValueType = RID;

// 节点基类
struct BTreeNode {
    PageID page_id;
    bool is_leaf;
    int size;  // 当前关键字数量
    PageID parent;

    BTreeNode(PageID pid = INVALID_PAGE_ID, bool leaf = false)
        : page_id(pid), is_leaf(leaf), size(0), parent(INVALID_PAGE_ID) {}
    virtual ~BTreeNode() = default;

    virtual void Serialize(char* data) const = 0;
    virtual void Deserialize(const char* data) = 0;
};

// 叶子节点
struct BTreeLeafNode : BTreeNode {
    std::vector<std::pair<KeyType, ValueType>> entries;
    PageID prev;  // 前向指针
    PageID next;  // 后向指针

    BTreeLeafNode(PageID pid = INVALID_PAGE_ID)
        : BTreeNode(pid, true), prev(INVALID_PAGE_ID), next(INVALID_PAGE_ID) {}

    void Serialize(char* data) const override;
    void Deserialize(const char* data) override;
};

// 内部节点
struct BTreeInternalNode : BTreeNode {
    std::vector<KeyType> keys;
    std::vector<PageID> children;

    BTreeInternalNode(PageID pid = INVALID_PAGE_ID)
        : BTreeNode(pid, false) {}

    void Serialize(char* data) const override;
    void Deserialize(const char* data) override;
};

class BTreeIndex {
private:
    BufferPool* buffer_pool_;
    PageID root_page_id_;
    int order_;  // 阶数：每个节点最多order_-1个关键字
    PageID next_page_id_;  // 用于分配新页ID

    // 辅助函数
    std::unique_ptr<BTreeNode> FetchNode(PageID pid);
    void SaveNode(const BTreeNode* node);
    PageID AllocatePage();
    bool InsertHelper(PageID pid, const KeyType& key, const ValueType& value, KeyType& split_key, PageID& new_page_id);
    bool DeleteHelper(PageID pid, const KeyType& key);
    void SplitLeafNode(const BTreeLeafNode* leaf, BTreeLeafNode* new_leaf, KeyType& split_key);
    void SplitInternalNode(const BTreeInternalNode* internal, BTreeInternalNode* new_internal, KeyType& split_key);
    PageID FindFirstLeaf(const KeyType& key);

public:
    BTreeIndex(BufferPool* bp, int order = 100)
        : buffer_pool_(bp), root_page_id_(INVALID_PAGE_ID), order_(order), next_page_id_(0) {
        // 初始化根节点（如果是新树）
        if (root_page_id_ == INVALID_PAGE_ID) {
            root_page_id_ = AllocatePage();
            auto root = std::make_unique<BTreeLeafNode>(root_page_id_);
            SaveNode(root.get());
        }
    }

    bool Insert(const KeyType& key, const ValueType& value);
    bool Search(const KeyType& key, ValueType& value);
    bool Delete(const KeyType& key);
    std::vector<ValueType> RangeScan(const KeyType& start, const KeyType& end);
};

} // namespace lightdb

#endif // LIGHTDB_BPLUS_TREE_H