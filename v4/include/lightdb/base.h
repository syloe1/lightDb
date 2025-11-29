// include/lightdb/base.h
#ifndef LIGHTDB_BASE_H
#define LIGHTDB_BASE_H

#include <cstdint>
#include <string>
#include <vector>

namespace lightdb {
    using PageID = int32_t;
    const PageID INVALID_PAGE_ID = -1; //invalid page 
    const int PAGE_SIZE = 4096;
    struct RID {
        PageID page_id;
        int slot_id; //index position in page record

        RID() : page_id(INVALID_PAGE_ID), slot_id(-1) {}
        RID(PageID pid, int sn) : page_id(pid), slot_id(sn) {}

        bool operator==(const RID& other) const {
            return page_id==other.page_id && slot_id==other.slot_id;
        }
        std::string ToString() const {
            return "RID{page_id: " + std::to_string(page_id) + ", slot_id: " + std::to_string(slot_id) + "}";
        }
    };
    
    struct Tuple {
        std::vector<std::string> fields;

        void AddField(const std::string& field) {
            fields.push_back(field);
        }

        std::string GetField(int idx) const {
            if(idx >= 0 && idx < fields.size()) {
                return fields[idx];
            }
            return "";
        }
    };
}// namespace lightdb
#endif // LIGHTDB_BASE_H