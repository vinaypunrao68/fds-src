/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Header class that defines the FDS lookup/placement
 * table structures.
 */

#ifndef SOURCE_INCLUDE_FDS_PLACEMENT_TABLE_H_
#define SOURCE_INCLUDE_FDS_PLACEMENT_TABLE_H_

#include <unordered_map>
#include <cmath>
#include <vector>

#include <fds_types.h>
#include <fds_err.h>
/*
 * TODO: Needed to create the copy
 * constructor from ICE network
 * table type.
 */
// #include "fdsp/fds_pubsub.h"
#include <fdsp/FDSP.h>

namespace fds {

  typedef fds_uint64_t fds_nodeid_t;

  class fds_placement_table {
 private:
    /*
     * Define the size of the table and
     * the size of table keys.
     */
    fds_uint32_t width;
    fds_uint32_t num_buckets;
    fds_uint32_t max_key;
    fds_uint32_t max_depth;
    fds_uint64_t version;

    /*
     * Hash table that maps to a list of nodes.
     * TODO: Make the list map to nodes, not IDs.
     */
    std::unordered_map<fds_uint32_t, std::vector<fds_nodeid_t>> table;

 public:
    /*
     * Constructors/destructors
     */
    explicit fds_placement_table(fds_uint32_t _width,
                                 fds_uint32_t _max_depth) :
    width(_width),
        num_buckets(pow(2, _width)),
        max_key(pow(2, _width) - 1),
        max_depth(_max_depth),
        version(0) {
    }

    explicit fds_placement_table(
        const FDS_ProtocolInterface::Node_Table_Type& ice_dlt) {
      for (fds_uint32_t i = 0; i < ice_dlt.size(); i++) {
        std::vector<fds_nodeid_t> node_list;
        table[i] = node_list;
        for (fds_uint32_t j = 0; j < ice_dlt[i].size(); j++) {
          /*
           * Copy the ice dlt nodeid entry into
           * the local dlt.
           */
          table[i].push_back(ice_dlt[i][j]);
        }
      }
    }

    ~fds_placement_table() {
    }

    /*
     * Accessors
     */
    fds_uint32_t getWidth() const {
      return width;
    }
    fds_uint32_t getNumBuckets() const {
      return num_buckets;
    }
    fds_uint32_t getMaxKey() const {
      return max_key;
    }
    fds_uint32_t getMaxDepth() const {
      return max_depth;
    }

    fds_uint64_t getVersion() const {
      return version;
    }

    fds_uint32_t getRows() const {
      return table.size();
    }

    /*
     * Returns an Ice version of this table
     */
    FDS_ProtocolInterface::Node_Table_Type toIce() const {
      FDS_ProtocolInterface::Node_Table_Type iceTable;
      
      for (std::unordered_map<fds_uint32_t,
               std::vector<fds_nodeid_t>>::const_iterator it = table.cbegin();
           it != table.cend();
           it++) {
        FDS_ProtocolInterface::Node_List_Type node_vect;
        for (fds_uint32_t i = 0; i < (it->second).size(); i++) {
          node_vect.push_back((it->second)[i]);
        }
        iceTable.push_back(node_vect);
      }

      return iceTable;
    }

    /*
     * Operators
     */
    fds_bool_t operator==(const fds_placement_table& rhs) {
      /*
       * Verify all fields match
       */
      if ((width != rhs.width) ||
          (num_buckets != rhs.num_buckets) ||
          (max_key != rhs.max_key) ||
          (max_depth != rhs.max_depth) ||
          (version != rhs.version) ||
          (table.size() != rhs.table.size())) {
        return false;
      }

      /*
       * Verify all of the table entries match
       */
      for (std::unordered_map<fds_uint32_t,
               std::vector<fds_nodeid_t>>::const_iterator it = table.cbegin();
           it != table.cend();
           it++) {
        const std::vector<fds_nodeid_t>& nodes = it->second;
        const std::vector<fds_nodeid_t>& rhs_nodes = rhs.table.at(it->first);
        for (fds_uint32_t i = 0; i < nodes.size(); i++) {
          if (nodes[i] != rhs_nodes[i]) {
            return false;
          }
        }
      }
      return true;
    }

    fds_placement_table& operator=(const fds_placement_table& rhs) {
      width       = rhs.width;
      num_buckets = rhs.num_buckets;
      max_key     = rhs.max_key;
      max_depth   = rhs.max_depth;
      version     = rhs.version;
      table       = rhs.table;

      return *this;
    }

    Error insert(fds_uint32_t key, const std::vector<fds_nodeid_t>& nodes) {
      Error err(ERR_OK);

      if (key > max_key) {
        err = ERR_INVALID_ARG;
        return err;
      } else if (nodes.size() > max_depth) {
        err = ERR_INVALID_ARG;
        return err;
      }

      /*
       * This does a full copy of the enture list
       * into the table.
       * Currently, we'll overwrite any previous entry.
       */
      table[key] = nodes;

      return err;
    }
    void clear() {
      table.clear();
    }
  };

  inline std::ostream& operator<<(std::ostream& out,
                                  const fds_placement_table& fpt) {
    return out << "Table: [version:" << fpt.getVersion()
               << ", width: " << fpt.getWidth()
               << ", buckets: " << fpt.getNumBuckets()
               << ", depth: " << fpt.getMaxDepth()
               << ", rows: " << fpt.getRows()
               << "]";
  }

  class FdsDlt : public fds_placement_table {
 private:
 public:
    FdsDlt(fds_uint32_t _width,
           fds_uint32_t _max_depth) :
    fds_placement_table(_width, _max_depth) {
    }

    explicit FdsDlt(const FDS_ProtocolInterface::Node_Table_Type& ice_dlt) :
    fds_placement_table(ice_dlt) {
    }

    using fds_placement_table::insert;
    using fds_placement_table::clear;

    /*
     * A new Ice DLT pointer is allocated in this function.
     * Though it is up to caller to free it.
     */
    FDS_ProtocolInterface::FDSP_DLT_TypePtr toIce() const {
      FDS_ProtocolInterface::FDSP_DLT_TypePtr iceDlt =
          new FDS_ProtocolInterface::FDSP_DLT_Type;
      iceDlt->DLT_version = fds_placement_table::getVersion();
      iceDlt->DLT = fds_placement_table::toIce();
      return iceDlt;
    }
  };

  inline std::ostream& operator<<(std::ostream& out,
                                  const FdsDlt& dlt) {
    return out << "Data Lookup "
               << static_cast<const fds_placement_table&>(dlt);
  }

  class FdsDmt : public fds_placement_table {
 private:
 public:
    FdsDmt(fds_uint32_t _width,
           fds_uint32_t _max_depth) :
    fds_placement_table(_width, _max_depth) {
    }

    explicit FdsDmt(const FDS_ProtocolInterface::Node_Table_Type& ice_dmt) :
    fds_placement_table(ice_dmt) {
    }

    using fds_placement_table::insert;
    using fds_placement_table::clear;
    
    /*
     * A new Ice DLT pointer is allocated in this function.
     * Though it is up to caller to free it.
     */
    FDS_ProtocolInterface::FDSP_DMT_TypePtr toIce() const {
      FDS_ProtocolInterface::FDSP_DMT_TypePtr iceDlt =
          new FDS_ProtocolInterface::FDSP_DMT_Type;
      iceDlt->DMT_version = fds_placement_table::getVersion();
      iceDlt->DMT = fds_placement_table::toIce();
      return iceDlt;
    }
  };

  inline std::ostream& operator<<(std::ostream& out,
                                  const FdsDmt& dmt) {
    return out << "Data Management "
               << static_cast<const fds_placement_table&>(dmt);
  }
}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_PLACEMENT_TABLE_H_
