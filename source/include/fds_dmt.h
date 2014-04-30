/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_FDS_DMT_H_
#define SOURCE_INCLUDE_FDS_DMT_H_

#include <unordered_map>
#include <vector>

#include <fds_types.h>
#include <fds_error.h>
#include <serialize.h>
#include <util/Log.h>

namespace fds {

    // TOD(Anna) will need to move DltTokenGroup class definition into
    // some common header from dlt.h and maybe rename it
    typedef DltTokenGroup DmtColumn;
    typedef boost::shared_ptr<DmtColumn> DmtColumnPtr;
    typedef boost::shared_ptr<std::vector<DmtColumnPtr>> DmtTablePtr;

    class DMT: serialize::Serializable, hasLogger {
  public:
        DMT(fds_uint32_t _width,
            fds_uint32_t _depth,
            fds_uint64_t _version);

        ~DMT();

        /**
         * Accessors
         */
        fds_uint64_t getVersion() const; /**< Gets DMT version */
        fds_uint32_t getDepth() const;  /**< Gets num replicas */

        /**
         * Get group of nodes in DLT column 'col_index' or for
         * given volume id 'volume_id'
         * @return array of Node uuids where the node under index 0
         * is the primary node
         */
        DmtColumnPtr getNodes(fds_uint32_t col_index) const;
        DmtColumnPtr getNodes(fds_volid_t volume_id) const;

        /**
         * Set the node group or particular cell in the DMT
         */
        void setNodes(fds_uint32_t col_index, const DmtColumnPtr& nodes);
        void setNode(fds_uint32_t col_index,
                     fds_uint32_t row_index,
                     const NodeUuid& uuid);

        /**
         * For serializing and de-serializing
         */
        uint32_t virtual write(serialize::Serializer* s) const;
        uint32_t virtual read(serialize::Deserializer* d);

        friend std::ostream& operator<< (std::ostream &out, const DMT& dmt);

  private:
        fds_uint64_t version;  /**< DMT version */
        fds_uint32_t depth;  /**< DMT column size */
        fds_uint32_t width;  /**< num columns = 2^width */

        DmtTablePtr dmt_table;  /**< DMT table */
    };

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_DMT_H_
