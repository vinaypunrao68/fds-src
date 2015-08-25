/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_FDS_DMT_H_
#define SOURCE_INCLUDE_FDS_DMT_H_

#include <string>
#include <vector>
#include <set>
#include <map>

#include <fds_types.h>
#include <fds_error.h>
#include <concurrency/RwLock.h>
#include <serialize.h>
#include <util/Log.h>
#include <fds_placement_table.h>
#include <fds_table.h>

namespace fds {

#define DMT_VER_INVALID 0  /**< Defines 0 as invalid DMT version */

    typedef TableColumn DmtColumn;
    typedef boost::shared_ptr<DmtColumn> DmtColumnPtr;
    typedef boost::shared_ptr<std::vector<DmtColumnPtr>> DmtTablePtr;

    class DMT: public FDS_Table {
  public:
        DMT(fds_uint32_t _width,
            fds_uint32_t _depth,
            fds_uint64_t _version,
            fds_bool_t alloc_cols = true);

        virtual ~DMT();

        /**
         * Accessors
         */
        fds_uint64_t getVersion() const; /**< Gets DMT version */
        fds_uint32_t getDepth() const;  /**< Gets num replicas */
        fds_uint32_t getNumColumns() const;  /**< Gets num columns */

        /**
         * Get node group that is responsible for given volume 'volume_id',
         * or volume range in the given DMT column 'col_index'
         * @return array of Node uuids where the node under index 0
         * is the primary node
         */
        DmtColumnPtr getNodeGroup(fds_uint32_t col_index) const;
        DmtColumnPtr getNodeGroup(fds_volid_t volume_id) const;
        /**
         * Returns column index in the DMT that is responsivle for
         * given volume id 'volume_id'.
         */
        static fds_uint32_t getNodeGroupIndex(fds_volid_t volid,
                                              fds_uint32_t num_columns);

        /**
         * Set the node group or particular cell in the DMT
         */
        void setNodeGroup(fds_uint32_t col_index, const DmtColumn& nodes);
        void setNode(fds_uint32_t col_index,
                     fds_uint32_t row_index,
                     const NodeUuid& uuid);

        /**
         * For serializing and de-serializing
         */
        uint32_t virtual write(serialize::Serializer* s) const;
        uint32_t virtual read(serialize::Deserializer* d);

        /**
         * Two DMTs are equal if they have the same content in the table
         * but versions may be different (or same).
         */
        fds_bool_t operator==(const DMT &rhs) const;
        friend std::ostream& operator<< (std::ostream &out, const DMT& dmt);

        /**
         * Print the DMT into the log. If log-level is debug, prints
         * node uuids in every cell.
         */
        void dump() const;

        /**
         * Checks if DMT is valid
         * Invalid cases:
         *    -- A column has repeating node uuids (non-unique)
         *    -- A cell in a DMT has an invalid Service UUID
         * @return ERR_OK of ERR_DMT_INVALID
         */
        Error verify() const;

        /**
         * Returns a set of nodes in DMT
         */
        void getUniqueNodes(std::set<fds_uint64_t>* ret_nodes) const;

        bool isVolumeOwnedBySvc(const fds_volid_t &volId, const fpi::SvcUuid &svcUuid) const;

  private:
        DmtTablePtr dmt_table;  /**< DMT table */
    };

    typedef boost::shared_ptr<DMT> DMTPtr;
    typedef enum {
        DMT_COMMITTED,
        DMT_TARGET,
        DMT_OLD,
    } DMTType;

    /**
     * Manages multiple DMT versions. Differentiates
     * between target and commited DMTs
     */
    class DMTManager {
  public:
        /**
         * DMTManager always able to maintain commited and target
         * DMTs. If 'history_dmts' > 0, then DMTManager will keep
         * extra history of 'history_dmts'
         */
        explicit DMTManager(fds_uint32_t history_dmts = 0);
        virtual ~DMTManager();

        Error add(DMT* dmt, DMTType dmt_type);
        Error addSerializedDMT(std::string& data, DMTType dmt_type);

        /**
         * Sets given version of DMT as commited
         * @param rmTarget if true, will set target version invalid. This
         * is a default behavior. If false, will keep target version valid
         * @return ERR_NOT_FOUND if target version does not exist
         */
        Error commitDMT(fds_bool_t rmTarget = true);
        /**
         * Set given version of DMT as committed
         * Will also unset Target DMT
         * If version is DMT_VER_INVALID, unsets commited DMT, and subsequent
         * calls to hasCommittedDMT() will return false.
         */
        Error commitDMT(fds_uint64_t version);
        /**
         * Sets target DMT version as invalid
         * @return ERR_NOT_FOUND if target version does not exist
         */
        Error unsetTarget(fds_bool_t rmTarget);
        inline fds_uint64_t getCommittedVersion() const {
            return committed_version;
        }
        inline fds_uint64_t getTargetVersion() const {
            return target_version;
        }
        inline fds_bool_t hasCommittedDMT() const {
            return (committed_version != DMT_VER_INVALID);
        };
        inline fds_bool_t hasTargetDMT() const {
            return (target_version != DMT_VER_INVALID);
        };

        /**
         * Reference counting for I/O against table versions
         */
        fds_uint64_t getAndLockCurrentVersion();
        void releaseVersion(const fds_uint64_t version);

        /**
         * Shortcut to get node group for a given volume 'volume_id'
         * from commited DMT. Asserts if there is no commited DMT
         */
        DmtColumnPtr getCommittedNodeGroup(fds_volid_t const volume_id) const;
        DmtColumnPtr getTargetNodeGroup(fds_volid_t const volume_id) const;
        DmtColumnPtr getVersionNodeGroup(fds_volid_t const volume_id,
                                         fds_uint64_t const version) const;

        /**
         * Returns DMT of given type, type must be either commited
         * or target. Asserts if there is no DMT of requested type.
         */
        DMTPtr getDMT(DMTType dmt_type);
        /**
         * Returns DMT of a given version; returns NULL if version
         * is not stored in DMTManager
         */
        DMTPtr getDMT(fds_uint64_t version);

        friend std::ostream& operator<< (std::ostream &out,
                                         const DMTManager& dmt_mgr);

  private:
        fds_uint32_t max_dmts;
        fds_uint64_t committed_version;  /**< version of commited DMT */
        fds_uint64_t target_version;  /**< version of target DMT or invalid */
        std::map<fds_uint64_t, DMTPtr> dmt_map;  /**< version to DMT map */
        mutable fds_rwlock dmt_lock;  /**< lock protecting dmt_map */
    };
    typedef boost::shared_ptr<DMTManager> DMTManagerPtr;


}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_DMT_H_
