/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_PLACEMENT_TABLE_H_
#define SOURCE_INCLUDE_FDS_PLACEMENT_TABLE_H_

#include <string>
#include <fds_typedefs.h>
#include <fds_defines.h>
#include <fds_resource.h>

namespace fds {

    /**
     * Class to store an array of nodeuuids that are part of a
     * the column in the table. Had to do this for shared_ptr.
     * will consider shared_array later.
     */
    class TableColumn {
  public:
        explicit TableColumn(fds_uint32_t len)
                : length(len) {
            // Create an array of uuid set to 0
            p = new NodeUuid[length]();
        }
        explicit TableColumn(const TableColumn& n) {
            length = n.length;
            p = new NodeUuid[length];
            // TODO(Prem): change to memcpy later
            for (fds_uint32_t i = 0; i < length; i++) {
                p[i] = n.p[i];
            }
        }
        inline void set(fds_uint32_t index, NodeUuid uid) {
            fds_verify(index < length);
            p[index] = uid;
        }
        inline NodeUuid get(fds_uint32_t index) const {
            fds_verify(index < length);
            return p[index];
        }

        NodeUuid& operator[] (int index) {
            return p[index];
        }
        inline fds_bool_t operator==(const TableColumn &rhs) const {
            if (length != rhs.length) {
                return false;
            }
            for (uint32_t i = 0; i < length; i++) {
                if (p[i] != rhs.p[i]) {
                    return false;
                }
            }
            return true;
        }

        /**
         * Returns the row # of the column of which the Node resides.
         * Returns -1 if not found.
         */
        int find(const NodeUuid &uid) const {
            for (uint32_t i = 0; i < length; i++) {
                if (p[i] == uid) {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }

        inline fds_uint32_t getLength() const {
            return length;
        }
        ~TableColumn() {
            if (p) {
                delete[] p;
            }
        }
        friend std::ostream& operator<< (std::ostream &out,
                                         const TableColumn& column);

  private:
        NodeUuid     *p;
        fds_uint32_t length;
    };
}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_PLACEMENT_TABLE_H_
