/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <ostream>
#include <sstream>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <string>

#include <fds_dmt.h>

namespace fds {

DMT::DMT(fds_uint32_t _width,
         fds_uint32_t _depth,
         fds_uint64_t _version,
         fds_bool_t alloc_cols)
        : FDS_Table(_width, _depth, _version),
          dmt_table(new std::vector<DmtColumnPtr>()) {
    // preallocate DMT and fill with invalid Node
    if (alloc_cols) {
        dmt_table->reserve(columns);
        for (fds_uint32_t i = 0; i < columns; ++i) {
            dmt_table->push_back(DmtColumnPtr(new DmtColumn(depth)));
        }
    }
}

DMT::~DMT() {
}

/**
 * Returns the version of this DMT
 */
fds_uint64_t DMT::getVersion() const {
    return version;
}

/**
 * Returns the depth (number of rows) of this DMT
 */
fds_uint32_t DMT::getDepth() const {
    return depth;
}

/**
 * Returns number of columns in this DMT
 */
fds_uint32_t DMT::getNumColumns() const {
    return columns;
}

/**
 * Returns node uuids in the DMT column 'col_index'
 */
DmtColumnPtr DMT::getNodeGroup(fds_uint32_t col_index) const {
    fds_verify(col_index < columns);
    return dmt_table->at(col_index);
}

/**
 * Returns a list of DM uuids responsible for this volume_id
 * where the node uuid [0] is the primary
 */
DmtColumnPtr DMT::getNodeGroup(fds_volid_t volume_id) const {
    fds_verify(columns > 0);
    fds_uint32_t col_index = volume_id.get() % columns;
    return dmt_table->at(col_index);
}

/**
 * returns column index for given 'volume_id' for a given number of
 * columns in DMT
 * @param num_columns is number of columns in DMT, must be > 0
 */
fds_uint32_t DMT::getNodeGroupIndex(fds_volid_t volume_id,
                                    fds_uint32_t num_columns) {
    fds_verify(num_columns > 0);
    return (volume_id.get() % num_columns);
}

/**
 * Sets the column 'col_index' -- overwrites if already set
 */
void DMT::setNodeGroup(fds_uint32_t col_index, const DmtColumn& nodes) {
    fds_verify(nodes.getLength() == depth);
    for (fds_uint32_t i = 0; i < depth; ++i) {
        (dmt_table->at(col_index))->set(i, nodes.get(i));
    }
}

/**
 * Sets the cell col_index, row_index -- overwrites if already set
 */
void DMT::setNode(fds_uint32_t col_index,
                  fds_uint32_t row_index,
                  const NodeUuid& uuid) {
    fds_verify(col_index < columns);
    fds_verify(row_index < depth);
    dmt_table->at(col_index)->set(row_index, uuid);
}

uint32_t DMT::write(serialize::Serializer* s) const {
    LOGNORMAL << "Serializing DMT version " << version;
    uint32_t b = 0;

    b += s->writeI64(version);
    b += s->writeI32(depth);
    b += s->writeI32(width);
    // We will calculate columns from width when de-serializing

    // We will do direct tranfer of each column since we expect
    // DMT table be quite small (8 columns or a bit more); if
    // we have large DMT later, we need to do smth similar to
    // DLT serialization.
    std::vector<DmtColumnPtr>::const_iterator it;
    for (it = dmt_table->cbegin();
         it != dmt_table->cend();
         ++it) {
        for (fds_uint32_t i = 0; i < depth; ++i) {
            NodeUuid uuid = (*it)->get(i);
            b += s->writeI64(uuid.uuid_get_val());
        }
    }
    return b;
}

uint32_t DMT::read(serialize::Deserializer* d) {
    uint32_t b = 0;
    fds_uint64_t i64;

    b += d->readI64(version);
    b += d->readI32(depth);
    b += d->readI32(width);
    columns = pow(2, width);

    LOGNORMAL << "De-serializing DMT version " << version;

    dmt_table->clear();
    dmt_table->reserve(columns);
    for (fds_uint32_t i = 0; i < columns; ++i) {
        dmt_table->push_back(DmtColumnPtr(new DmtColumn(depth)));
        for (fds_uint32_t j = 0; j < depth; ++j) {
            b += d->readI64(i64);
            dmt_table->at(i)->set(j, NodeUuid(i64));
        }
    }
    return b;
}

void DMT::dump() const {
    LEVELCHECK(debug) {
        std::set<fds_uint64_t> nodes;
        getUniqueNodes(&nodes);
        LOGDEBUG << (*this);
        LOGDEBUG << "Nodes in DMT: " << nodes.size();
        for (std::set<fds_uint64_t>::const_iterator cit = nodes.cbegin();
             cit != nodes.cend();
             ++cit) {
            LOGDEBUG << std::hex << *cit << std::dec;
        }
    } else {
        LOGNORMAL << "DMT: "
                  << "[version: " << version << "] "
                  << "[depth: " << depth << "] "
                  << "[width: " << width << "] "
                  << "[columns: " << columns << "] ";
    }
}

std::ostream& operator<< (std::ostream &oss, const DMT& dmt) {
    std::vector<DmtColumnPtr>::const_iterator it;
    oss << "DMT: "
        << "[version: " << dmt.version << "] "
        << "[depth: " << dmt.depth << "] "
        << "[width: " << dmt.width << "] "
        << "[columns: " << dmt.columns << "]\n";

    fds_uint32_t j = 0;
    for (it = dmt.dmt_table->cbegin();
         it != dmt.dmt_table->cend();
         ++it) {
        oss << "col " << j << ":  ";
        for (fds_uint32_t i = 0; i < dmt.depth; ++i) {
            NodeUuid uuid = (*it)->get(i);
            oss << " 0x" << std::hex << uuid.uuid_get_val() << std::dec;
        }
        oss << "\n";
        ++j;
    }
    return oss;
}

Error DMT::verify() const {
    Error err(ERR_OK);
    std::vector<DmtColumnPtr>::const_iterator it;
    for (it = dmt_table->cbegin();
         it != dmt_table->cend();
         ++it) {
        std::unordered_set<NodeUuid, UuidHash> nodes;
        for (fds_uint32_t i = 0; i < depth; ++i) {
            NodeUuid uuid = (*it)->get(i);
            if (uuid.uuid_get_val() == 0) {
                return ERR_INVALID_DMT;
            }
            nodes.insert(uuid);
        }
        // uuids in the same column must be unique
        if (nodes.size() != depth) {
            return ERR_INVALID_DMT;
        }
    }
    return err;
}

fds_bool_t DMT::operator==(const DMT &rhs) const {
    // number of rows and columns has to match
    if ((depth != rhs.depth) || (columns != rhs.columns)) {
        return false;
    }

    // every column should match
    for (fds_uint32_t i = 0; i < columns; ++i) {
        DmtColumnPtr myCol = dmt_table->at(i);
        DmtColumnPtr col = rhs.dmt_table->at(i);
        if ((myCol == nullptr) || (col == nullptr)) {
            return false;
        }
        if (!(*myCol == *col)) {
            return false;
        }
    }
    return true;
}

/**
 * returns a set of nodes in DMT
 */
void DMT::getUniqueNodes(std::set<fds_uint64_t>* ret_nodes) const {
    std::set<fds_uint64_t> nodes;
    std::vector<DmtColumnPtr>::const_iterator it;
    for (it = dmt_table->cbegin();
         it != dmt_table->cend();
         ++it) {
        for (fds_uint32_t i = 0; i < depth; ++i) {
            NodeUuid uuid = (*it)->get(i);
            nodes.insert(uuid.uuid_get_val());
        }
    }
    (*ret_nodes).swap(nodes);
}

bool DMT::isVolumeOwnedBySvc(const fds_volid_t &volId, const fpi::SvcUuid &svcUuid) const {
    auto nodeGroup = getNodeGroup(volId);
    return (nodeGroup->find(NodeUuid(svcUuid)) != -1);
}

/***** DMTManager implementation ****/

DMTManager::DMTManager(fds_uint32_t history_dmts)
        : max_dmts(history_dmts+2),
          committed_version(DMT_VER_INVALID),
          target_version(DMT_VER_INVALID) {
}

DMTManager::~DMTManager() {
}

Error DMTManager::add(DMT* dmt, DMTType dmt_type) {
    Error err(ERR_OK);
    fds_uint64_t add_version = dmt->getVersion();

    dmt_lock.write_lock();

    // will not add version that already exists
    if (dmt_map.count(add_version) > 0) {
        dmt_lock.write_unlock();
        return ERR_DUPLICATE;
    }
    fds_verify(committed_version != add_version);
    fds_verify(target_version != add_version);

    if (dmt_type == DMT_COMMITTED) {
        committed_version = add_version;
    } else if (dmt_type == DMT_TARGET) {
        target_version = add_version;
    } else {
        fds_verify(false);  // must be committed or target
    }
    dmt_map[add_version] = DMTPtr(dmt);

    // If we have too many elements in a map, remove DMT
    // whith the oldest version
    if (dmt_map.size() > max_dmts) {
        std::map<fds_uint64_t, DMTPtr>::iterator it = dmt_map.begin();
        fds_uint64_t del_version = (it->second)->getVersion();
        fds_verify(del_version != committed_version);
        fds_verify(del_version != target_version);
        dmt_map.erase(it);
    }
    dmt_lock.write_unlock();
    return err;
}

Error DMTManager::addSerializedDMT(std::string& data, DMTType dmt_type) {
    Error err(ERR_OK);
    DMT *dmt = new DMT(0, 0, 0, false);
    err = dmt->loadSerialized(data);
    if (err.ok()) {
        err = add(dmt, dmt_type);
    }
    dmt->dump();
    return err;
}

/**
 * Commit target DMT version.
 * returns ERR_NOT_FOUND if target version does not exist
 */
Error DMTManager::commitDMT(fds_bool_t rmTarget) {
    Error err(ERR_OK);
    dmt_lock.write_lock();
    if (target_version != DMT_VER_INVALID) {
        fds_verify(dmt_map.count(target_version) > 0);
        committed_version = target_version;
        if (rmTarget) {
            target_version = DMT_VER_INVALID;
        }
    } else {
        err = ERR_NOT_FOUND;
    }
    dmt_lock.write_unlock();
    return err;
}

Error DMTManager::commitDMT(fds_uint64_t version) {
    Error err(ERR_OK);
    dmt_lock.write_lock();
    if (version != DMT_VER_INVALID) {
        if (dmt_map.count(version) > 0) {
            committed_version = version;
            target_version = DMT_VER_INVALID;
        } else {
            fds_verify(dmt_map.size() == 0);
            committed_version = DMT_VER_INVALID;
            target_version = DMT_VER_INVALID;
            err = ERR_NOT_FOUND;
        }
    } else {
        committed_version = DMT_VER_INVALID;
        target_version = DMT_VER_INVALID;
    }
    dmt_lock.write_unlock();
    return err;
}

Error DMTManager::unsetTarget(fds_bool_t rmTarget) {
    Error err(ERR_OK);
    dmt_lock.write_lock();
    if (target_version != DMT_VER_INVALID) {
        fds_verify(dmt_map.count(target_version) > 0);
        if (rmTarget) {
            // remove target DMT from the map 
            dmt_map.erase(target_version);
        }
        target_version = DMT_VER_INVALID;
    } else {
        err = ERR_NOT_FOUND;
    }
    dmt_lock.write_unlock();
    return err;
}

fds_uint64_t DMTManager::getAndLockCurrentVersion() {
    ReadGuard guard(dmt_lock);
    if (committed_version == DMT_VER_INVALID) {
        throw Exception(ERR_INVALID_DMT, "DMT is not valid");
    }
    auto const it = dmt_map.find(committed_version);
    fds_verify(dmt_map.cend() != it);
    it->second->incRefcnt();
    return committed_version;
}

void DMTManager::releaseVersion(const fds_uint64_t version) {
    ReadGuard guard(dmt_lock);
    if (committed_version == DMT_VER_INVALID) {
        throw Exception(ERR_INVALID_DMT, "DMT is not valid");
    }
    auto const it = dmt_map.find(committed_version);
    fds_verify(dmt_map.cend() != it);
    it->second->decRefcnt();
}

DmtColumnPtr DMTManager::getCommittedNodeGroup(fds_volid_t const vol_id) const {
    ReadGuard guard(dmt_lock);
    if (committed_version == DMT_VER_INVALID) {
        throw Exception(ERR_INVALID_DMT, "DMT is not valid");
    }
    auto const it = dmt_map.find(committed_version);
    fds_verify(dmt_map.cend() != it);
    return it->second->getNodeGroup(vol_id);
}

DmtColumnPtr DMTManager::getTargetNodeGroup(fds_volid_t const vol_id) const {
    ReadGuard guard(dmt_lock);
    fds_verify(target_version != DMT_VER_INVALID);
    auto const it = dmt_map.find(target_version);
    fds_verify(dmt_map.cend() != it);
    return it->second->getNodeGroup(vol_id);
}

DmtColumnPtr DMTManager::getVersionNodeGroup(fds_volid_t const volume_id,
                                             fds_uint64_t const version) const {
    ReadGuard guard(dmt_lock);
    fds_verify(version != DMT_VER_INVALID);
    auto const it = dmt_map.find(version);
    fds_verify(dmt_map.cend() != it);
    return it->second->getNodeGroup(volume_id);
}

DMTPtr DMTManager::getDMT(fds_uint64_t version) {
    dmt_lock.read_lock();
    if (dmt_map.count(version) > 0) {
        DMTPtr dmt = dmt_map[version];
        dmt_lock.read_unlock();
        return dmt;
    }
    dmt_lock.read_unlock();
    return DMTPtr();
}

DMTPtr DMTManager::getDMT(DMTType dmt_type) {
    fds_uint64_t ver = DMT_VER_INVALID;
    dmt_lock.read_lock();
    switch (dmt_type) {
        case DMT_COMMITTED:
            ver = committed_version;
            break;
        case DMT_TARGET:
            ver = target_version;
            break;
        default:
            fds_verify(false);
    };
    fds_verify(dmt_map.count(ver) > 0);
    DMTPtr dmt = dmt_map[ver];
    dmt_lock.read_unlock();
    return dmt;
}

std::ostream& operator<< (std::ostream &oss, const DMTManager& dmt_mgr) {
    oss << "DMTManager: "
        << "[commited version: " << dmt_mgr.committed_version << "] "
        << "[target version: " << dmt_mgr.target_version << "] "
        << "[number of dmts: " << dmt_mgr.dmt_map.size() << "]\n";

    if (dmt_mgr.committed_version != DMT_VER_INVALID) {
        oss << "Committed dmt "
            << *(dmt_mgr.dmt_map.at(dmt_mgr.committed_version)) << "\n";
    }
    if (dmt_mgr.target_version != DMT_VER_INVALID) {
        oss << "Target dmt "
            << *(dmt_mgr.dmt_map.at(dmt_mgr.target_version)) << "\n";
    }
    return oss;
}



}  // namespace fds
