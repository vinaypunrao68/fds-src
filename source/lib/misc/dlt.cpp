/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <cmath>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <dlt.h>
#include <iostream>
#include <string>
#include <util/Log.h>
#include <sstream>
#include <iomanip>
#include <ostream>
/**
 *  Implementationg for DLT ...
 */
namespace fds {

std::ostream& operator<< (std::ostream &out, const TableColumn& column) {
    out << "Node Group : ";
    for (uint i = 0; i < column.length; i++) {
        out << "[" << i << ":" << column.get(i).uuid_get_val() <<"] ";
    }
    return out;
}

DLT::DLT(fds_uint32_t width,
         fds_uint32_t depth,
         fds_uint64_t version,
         bool fInit)
        : FDS_Table(width, depth, version),
          Module("data placement table"),
          closed(false) {
    distList = boost::shared_ptr<std::vector<DltTokenGroupPtr> >
            (new std::vector<DltTokenGroupPtr>());
    mapNodeTokens = boost::shared_ptr<NodeTokenMap>(
        new NodeTokenMap());

    LOGDEBUG << "dlt.init : " << "numbits:" << width
             << " depth:" << depth << " version:" << version;
    // Pre-allocate token groups for each token
    if (fInit) {
        distList->reserve(columns);
        for (uint i = 0; i < columns; i++) {
            distList->push_back(boost::shared_ptr<DltTokenGroup>(
                new DltTokenGroup(depth)));
        }
    }
    timestamp = fds::util::getTimeStampMillis();
}

DLT::DLT(const DLT& dlt)
        : FDS_Table(dlt.width, dlt.depth, dlt.version),
          Module("data placement table"),
          distList(dlt.distList),
          timestamp(dlt.timestamp),
          closed(false),
          mapNodeTokens(dlt.mapNodeTokens) {
}

DLT* DLT::clone() const {
    LOGTRACE << "cloning";
    DLT *pDlt = new DLT(width, depth, version, true);

    pDlt->version = version;
    pDlt->timestamp = timestamp;

    for (uint i = 0; i < columns; i++) {
        for (uint j = 0 ; j < depth ; j++) {
            pDlt->distList->at(i)->set(j, distList->at(i)->get(j));
        }
    }
    return pDlt;
}

int DLT::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

void DLT::mod_startup() {
}

void DLT::mod_shutdown() {
}

fds_uint64_t
DLT::getVersion() const {
    return version;
}

fds_uint32_t
DLT::getDepth() const {
    return depth;
}

fds_uint32_t DLT::getWidth() const {
    return getNumBitsForToken();
}

fds_uint32_t DLT::getNumBitsForToken() const {
    return width;
}

fds_uint32_t DLT::getNumTokens() const {
    return columns;
}

fds_bool_t DLT::isClosed() const {
    return closed;
}

void DLT::setClosed() {
    closed = true;
}

fds_token_id DLT::getToken(const ObjectID& objId) const {
    fds_uint64_t token_bitmask = ((1 << width) - 1);
//    fds_uint64_t bit_offset =
//        (sizeof(objId.getTokenBits(width))*8 - width);
    LOGDEBUG << "token bits:" << objId.getTokenBits(width)
             << " numBits: " << width;
    return
     (fds_token_id)(token_bitmask & (objId.getTokenBits(width)));
#if 0
    fds_uint64_t token_bitmask = ((1 << width) - 1);
    fds_uint64_t bit_offset = (sizeof(objId.GetHigh())*8 - width);
    return (fds_token_id)(token_bitmask & (objId.GetHigh() >> bit_offset));
#endif
}

fds_token_id DLT::getToken(const ObjectID& objId,
                          fds_uint32_t bitsPerToken) {
    fds_verify(bitsPerToken > 0);
    fds_uint64_t token_bitmask = ((1 << bitsPerToken) - 1);
    return (fds_token_id)(token_bitmask &
                          (objId.getTokenBits(bitsPerToken)));
}

void DLT::getTokenObjectRange(const fds_token_id &token,
        ObjectID &begin, ObjectID &end) const
{
    ObjectID::getTokenRange(token, width, begin, end);
}

// get all the Nodes for a token/objid
DltTokenGroupPtr DLT::getNodes(fds_token_id token) const {
    fds_verify(token < columns);
    return distList->at(token);
}

DltTokenGroupPtr DLT::getNodes(const ObjectID& objId) const {
    fds_token_id token = getToken(objId);
    fds_verify(token < columns);
    return distList->at(token);
}

// get the primary node for a token/objid
NodeUuid DLT::getPrimary(fds_token_id token) const {
    fds_verify(token < columns);
    return getNodes(token)->get(0);
}

NodeUuid DLT::getPrimary(const ObjectID& objId) const {
    return getNodes(objId)->get(0);
}

NodeUuid DLT::getNode(fds_token_id token, uint index) const {
    fds_verify(token < columns);
    return getNodes(token)->get(index);
}

NodeUuid DLT::getNode(const ObjectID& objId, uint index) const {
    return getNodes(objId)->get(index);
}

int DLT::getIndex(const fds_token_id& token, const NodeUuid& nodeUuid) const {
    return getNodes(token)->find(nodeUuid);
}

int DLT::getIndex(const ObjectID& objId, const NodeUuid& nodeUuid) const {
    return getNodes(objId)->find(nodeUuid);
}

void DLT::setNode(fds_token_id token, uint index, NodeUuid nodeuuid) {
    fds_verify(index < depth);
    fds_verify(token < columns);
    distList->at(token)->set(index, nodeuuid);
}

void DLT::setNodes(fds_token_id token, const DltTokenGroup& nodes) {
    fds_verify(nodes.getLength() == depth);
    for (uint i = 0; i < depth ; i++) {
        distList->at(token)->set(i, nodes.get(i));
    }
}

void DLT::generateNodeTokenMap() const {
    LOGDEBUG << "generating node-token map";
    std::vector<DltTokenGroupPtr>::const_iterator iter;
    mapNodeTokens->clear();
    uint i;
    fds_token_id token = 0;
    for (iter = distList->begin(); iter != distList->end(); iter++) {
        const DltTokenGroupPtr& nodeList= *iter;
        for (i = 0; i < depth; i++) {
            (*mapNodeTokens)[nodeList->get(i)].push_back(token);
        }
        token++;
    }
}

// get the Tokens for a given Node
const TokenList& DLT::getTokens(const NodeUuid &uid) const {
    static TokenList emptyTokenList;
    NodeTokenMap::const_iterator iter = mapNodeTokens->find(uid);
    if (iter != mapNodeTokens->end()) {
        return iter->second;
    } else {
        // TODO(prem) : need to revisit this
        return emptyTokenList;
    }
}

// get the Tokens for a given Node at a specific index
void DLT::getTokens(TokenList* tokenList, const NodeUuid &uid, uint index) const {
    NodeTokenMap::const_iterator iter = mapNodeTokens->find(uid);
    if (iter != mapNodeTokens->end()) {
        TokenList::const_iterator tokenIter;
        NodeUuid curNode;
        const TokenList& tlist = iter->second;
        for (tokenIter = tlist.begin(); tokenIter != tlist.end(); ++tokenIter) {
            curNode = distList->at(*tokenIter)->get(index);
            if (curNode == uid) {
                tokenList->push_back(*tokenIter);
            }
        }
    }
}

/**
 * Get source node for a given DLT token assigned to the destination node.
 */
NodeUuid DLT::getSourceNodeForToken(const NodeUuid &destNodeUuid,
                                    const fds_token_id &tokenId) const {
    NodeUuid srcNodeUuid(INVALID_RESOURCE_UUID);
    DltTokenGroupPtr tokenNodeGroup = getNodes(tokenId);

    if (destNodeUuid != tokenNodeGroup->get(sm1PIdx)) {
        srcNodeUuid = tokenNodeGroup->get(sm1PIdx);
    } else if (tokenNodeGroup->getLength() > sm2PIdx) {
        srcNodeUuid = tokenNodeGroup->get(sm2PIdx);
    }

    return srcNodeUuid;
}
// get source nodes for all the tokens of a given destination node
void DLT::getSourceForAllNodeTokens(const NodeUuid &nodeUuid,
                                    SourceNodeMap &srcNodeTokenMap) const {
    if (nodeUuid == INVALID_RESOURCE_UUID) {
        LOGERROR << "Invalid node uuid";
        return;
    }
    const TokenList& nodeTokenList = getTokens(nodeUuid);
    for (TokenList::const_iterator tokenIter = nodeTokenList.begin();
         tokenIter != nodeTokenList.end();
         tokenIter++) {
        srcNodeTokenMap[getSourceNodeForToken(nodeUuid, *tokenIter)].push_back(*tokenIter);
    }
}

/**
 * Returns a map of new source SMs for a given set of dlt tokens.
 * For every retry, this method will return a different
 * source SM per token. Caller should make sure that it is
 * not retrying more than N-1 times for token stored with replica
 * count as N.
 */
NodeTokenMap DLT::getNewSourceSMs(const NodeUuid&  curSrcSM,
                                  const std::set<fds_token_id>& dltTokens,
                                  const uint8_t& retryCount,
                                  std::map<NodeUuid, bool>& failedSMs) const {
    NodeTokenMap newTokenGroups;
    /**
     * Go over the table column for the token in DLT table and figure
     * out the next replica SM which will act as source for migration of
     * this token.
     */
    for (std::set<fds_token_id>::iterator tokenIter = dltTokens.begin();
         tokenIter != dltTokens.end(); tokenIter++) {
        bool foundSrcSM = false;
        uint8_t curSrcIdx = getIndex(*tokenIter, curSrcSM);
        uint8_t newSrcIdx = (curSrcIdx + retryCount) % getDepth();

        while (!foundSrcSM && newSrcIdx != curSrcIdx) {
            NodeUuid newSrcSmId = getNode(*tokenIter, newSrcIdx);
            if (newSrcIdx != sm1PIdx && newSrcIdx != sm2PIdx) {
                // must be one of the primaries
                ++newSrcIdx %= getDepth();
            } else if (failedSMs.find(newSrcSmId) == failedSMs.end()) {
                // Found a healthy source for migration of this dlt token.
                newTokenGroups[newSrcSmId].push_back(*tokenIter);
                foundSrcSM = true;
            } else {
                ++newSrcIdx %= getDepth();
            }
        }
        /**
         * Cannot find a healthy source SM for migration of this dlt token
         * since all replicas holding data for this dlt token are reported
         * as failed.
         */
        if (!foundSrcSM) {
            newTokenGroups[INVALID_RESOURCE_UUID].push_back(*tokenIter);
        }
    }

    return newTokenGroups;
}

fds_bool_t DLT::operator==(const DLT &rhs) const {
    // number of rows and columns has to match
    if ((depth != rhs.depth) || (columns != rhs.columns)) {
        return false;
    }

    // every column should match
    for (fds_token_id i = 0; i < columns; ++i) {
        DltTokenGroupPtr myCol = getNodes(i);
        DltTokenGroupPtr col = rhs.getNodes(i);
        if ((myCol == nullptr) && (col == nullptr)) {
            continue;  // ok
        } else if ((myCol == nullptr) || (col == nullptr)) {
            return false;
        }
        if (!(*myCol == *col)) {
            return false;
        }
    }
    return true;
}

Error DLT::verify(const NodeUuidSet& expectedUuidSet) const {
    Error err(ERR_OK);

    // we should not have more rows than nodes
    if (depth > expectedUuidSet.size()) {
        LOGERROR << "DLT has more rows (" << depth
                 << ") than nodes (" << expectedUuidSet.size() << ")";
        return ERR_INVALID_DLT;
    }

    // check each column in DLT
    NodeUuidSet colSet;
    for (fds_token_id i = 0; i < columns; ++i) {
        colSet.clear();
        DltTokenGroupPtr column = getNodes(i);
        for (fds_uint32_t j = 0; j < depth; ++j) {
            NodeUuid uuid = column->get(j);
            if ((uuid.uuid_get_val() == 0) ||
                (expectedUuidSet.count(uuid) == 0)) {
                // unexpected uuid in this DLT cell
                LOGERROR << "DLT contains unexpected uuid " << std::hex
                         << uuid.uuid_get_val() << std::dec;
                return ERR_INVALID_DLT;
            }
            colSet.insert(uuid);
        }

        // make sure that column contains all unique uuids
        if (colSet.size() < depth) {
            LOGERROR << "Found non-unique uuids in DLT column " << i;
            return ERR_INVALID_DLT;
        }
    }

    return err;
}

void DLT::dump() const {
    // go thru with the fn iff loglevel is debug or lesser.

    LEVELCHECK(debug) {
        LOGDEBUG << (*this);
    } else {
        LOGNORMAL<< "Dlt :"
                 << "[version: " << version  << "] "
                 << "[timestamp: " << timestamp  << "] "
                 << "[depth: " << depth  << "] "
                 << "[num.Tokens: " << columns  << "] "
                 << "[num.Nodes: " << mapNodeTokens->size() << "]";
    }
}

std::ostream& operator<< (std::ostream &oss, const DLT& dlt) {
    oss << "[version:" << dlt.version
        << " timestamp:" << dlt.timestamp
        << " depth:" << dlt.depth
        << " num.Tokens:" << dlt.columns
        << " num.Nodes:" << dlt.mapNodeTokens->size()
        << "]\n";

    oss << "token -> nodegroups :> \n";
    // print the whole dlt
    std::vector<DltTokenGroupPtr>::const_iterator iter;
    std::ostringstream ossNodeList;
    uint count = 0;
    std::string prevNodeListStr, curNodeListStr;
    fds_uint64_t token = 0, prevToken = 0 , firstToken = 0;

    size_t listSize = dlt.distList->size()-1;
    for (iter = dlt.distList->begin(); iter != dlt.distList->end(); iter++, count++) {
        const DltTokenGroupPtr& nodeList= *iter;
        ossNodeList.str("");
        for (uint i = 0; i < dlt.getDepth(); i++) {
            ossNodeList << nodeList->get(i).uuid_get_val();
            if (i != dlt.getDepth()-1) {
                ossNodeList << ",";
            }
        }
        curNodeListStr = ossNodeList.str();
        if (curNodeListStr != prevNodeListStr) {
            if (!prevNodeListStr.empty()) {
                if (firstToken != prevToken) {
                    oss << std::setw(8)
                        << firstToken
                        <<" - " << prevToken <<" : " << std::hex << prevNodeListStr << std::dec << "\n";
                } else {
                    oss << std::setw(8)
                        << firstToken
                        << " : " << std::hex << prevNodeListStr << "\n";
                }
                firstToken =  count;
            }
            prevNodeListStr = curNodeListStr;
        }
        prevToken = count;

        if (count == listSize) {
            // this is the end of the loop
            if (firstToken != prevToken) {
                oss << std::setw(8)
                    << firstToken
                    <<" - " << prevToken <<" : " << std::hex << prevNodeListStr << std::dec << "\n";
            } else {
                oss << std::setw(8)
                    << firstToken
                    << " : " << std::hex << prevNodeListStr << std::dec << "\n";
            }
        }
    }

    oss << "node -> token map :> \n";
    NodeTokenMap::const_iterator tokenMapiter;

    // build the unique uuid list
    token = 0, prevToken = 0 , firstToken = 0;

    for (tokenMapiter = dlt.mapNodeTokens->begin(); tokenMapiter != dlt.mapNodeTokens->end(); ++tokenMapiter) { // NOLINT
        TokenList::const_iterator tokenIter;
        const TokenList& tlist = tokenMapiter->second;
        oss << std::setw(8) << std::hex << tokenMapiter->first.uuid_get_val() << std::dec << " : ";
        prevToken = 0;
        firstToken = 0;
        bool fFirst = true;
        size_t last = tlist.size() - 1;
        count = 0;
        for (tokenIter = tlist.begin(); tokenIter != tlist.end(); ++tokenIter, ++count) {
            token = *tokenIter;
            if (fFirst) {
                firstToken = token;
                oss << token;
                fFirst = false;
            } else if (token == (prevToken + 1)) {
                // do nothing .. this is a range
            } else {
                if (prevToken != firstToken) {
                    // this is a range
                    oss << "-" << prevToken;
                }
                oss << "," << token << "\n";
                firstToken = token;
            }
            prevToken = token;

            if (count == last) {
                // end of loop
                if (prevToken != firstToken) {
                    // this is a range
                    oss << "-" << prevToken << "\n";
                } else {
                    oss << "\n";
                }
            }
        }
    }

    return oss;
}

uint32_t DLT::write(serialize::Serializer*  s) const {
    LOGDEBUG << "serializing dlt";
    uint32_t b = 0;

    b += s->writeI64(version);
    b += s->writeTimeStamp(timestamp);
    b += s->writeI32(width);
    b += s->writeI32(depth);
    b += s->writeI32(columns);


    std::call_once(mapInitialized,
                   [this] { if (this->mapNodeTokens->empty()) this->generateNodeTokenMap(); });

    typedef std::map<fds_uint64_t, uint32_t> UniqueUUIDMap;
    std::vector<fds_uint64_t> uuidList;
    UniqueUUIDMap uuidmap;

    NodeTokenMap::const_iterator tokenMapiter;

    // build the unique uuid list
    uint32_t count = 0;
    fds_uint64_t uuid;
    for (tokenMapiter = mapNodeTokens->begin(); tokenMapiter != mapNodeTokens->end(); ++tokenMapiter, ++count) { //NOLINT
        uuid = tokenMapiter->first.uuid_get_val();
        uuidmap[uuid] = count;
        uuidList.push_back(uuid);
    }
    //std::cout<<"count:"<<count<<": listsize:" <<uuidList.size()<<std::endl; // NOLINT
    // write the unique Node list
    // size of the node list
    b += s->writeI32(count);
    for (auto uuid : uuidList) {
        b += s->writeI64(uuid);
    }

    std::vector<DltTokenGroupPtr>::const_iterator iter;
    uint32_t lookupid;
    // with a byte we can support 256  unique Nodes
    // with 16-bit we can support 65536 unique Nodes
    bool fByte = (count <= 256);

    for (iter = distList->begin(); iter != distList->end(); iter++) {
        const DltTokenGroupPtr& nodeList= *iter;
        for (uint i = 0; i < depth; i++) {
            uuid = nodeList->get(i).uuid_get_val();
            lookupid = uuidmap[uuid];
            if (fByte) {
                b += s->writeByte(lookupid);
            } else {
                b += s->writeI16(lookupid);
            }
        }
    }
    return b;
}

uint32_t DLT::read(serialize::Deserializer* d) {
    LOGDEBUG << "de serializing dlt";
    uint32_t b = 0;
    int64_t i64;
    b += d->readI64(version);
    b += d->readTimeStamp(timestamp);
    b += d->readI32(width);
    b += d->readI32(depth);
    b += d->readI32(columns);

    fds_uint64_t uuid;
    distList->clear();
    distList->reserve(columns);

    uint32_t count = 0;
    std::vector<NodeUuid> uuidList;

    // read the Unique Node List
    b += d->readI32(count);
    uuidList.reserve(count);
    for (uint i = 0; i < count ; i++) {
        b += d->readI64(uuid);
        uuidList.push_back(NodeUuid(uuid));
    }

    bool fByte = (count <= 256);
    fds_uint8_t i8;
    fds_uint16_t i16;

    std::vector<DltTokenGroupPtr>::const_iterator iter;
    for (uint i = 0; i < columns ; i++) {
        DltTokenGroupPtr ptr = boost::shared_ptr<DltTokenGroup>(new DltTokenGroup(depth));
        for (uint j = 0; j < depth; j++) {
            if (fByte) {
                b += d->readByte(i8);
                ptr->set(j, uuidList.at(i8));
            } else {
                b += d->readI16(i16);
                ptr->set(j, uuidList.at(i16));
            }
        }
        distList->push_back(ptr);
    }

    return b;
}

bool DLT::loadFromFile(std::string filename) {
    LOGNOTIFY << "loading dltmgr from file : " << filename;
    serialize::Deserializer *d= serialize::getFileDeserializer(filename);
    read(d);
    delete d;
    return true;
}

bool DLT::storeToFile(std::string filename) {
    serialize::Serializer *s= serialize::getFileSerializer(filename);
    write(s);
    delete s;
    return true;
}

uint32_t DLT::getEstimatedSize() const {
    return 512*KB;
}

/**
 *
 * @param uid Node id
 * @param new_dlt
 * @param old_dlt
 * @return difference between new_dlt and old_dlt.  Only entries that exist in
 * new_dlt but don't exist in old_dlt are returned
 */
std::set<fds_token_id> DLT::token_diff(const NodeUuid &uid,
                const DLT* new_dlt, const DLT* old_dlt)
{
    std::set<fds_token_id> ret_set;
    if (old_dlt == nullptr) {
        ret_set.insert(new_dlt->getTokens(uid).begin(),
                new_dlt->getTokens(uid).end());
        return ret_set;
    }

    std::set<fds_token_id> old_set(old_dlt->getTokens(uid).begin(),
            old_dlt->getTokens(uid).end());
    std::set<fds_token_id> new_set(new_dlt->getTokens(uid).begin(),
                new_dlt->getTokens(uid).end());
    std::set_difference(new_set.begin(), new_set.end(),
            old_set.begin(), old_set.end(),
            std::insert_iterator<std::set<fds_token_id> >(ret_set, ret_set.end()));
    return ret_set;
}
//================================================================================

/**
 *  Implementation for DLT Diff
 */

DLTDiff::DLTDiff(DLT* baseDlt, fds_uint64_t version) {
    fds_verify(NULL != baseDlt);
    this->baseDlt = baseDlt;
    baseVersion = baseDlt->version;
    this->version = (version > 0)?version:baseVersion+1;
    timestamp = fds::util::getTimeStampMillis();
}

// get all the Nodes for a token/objid
DltTokenGroupPtr DLTDiff::getNodes(fds_token_id token) const {
    std::map<fds_token_id, DltTokenGroupPtr>::const_iterator iter;
    iter = mapTokenNodes.find(token);
    if (iter != mapTokenNodes.end()) return iter->second;

    return baseDlt->getNodes(token);
}

DltTokenGroupPtr DLTDiff::getNodes(const ObjectID& objId) const {
    return getNodes(baseDlt->getToken(objId));
}

// get the primary node for a token/objid
NodeUuid DLTDiff::getPrimary(fds_token_id token) const {
    return getNodes(token)->get(0);
}

NodeUuid DLTDiff::getPrimary(const ObjectID& objId) const {
    return getNodes(objId)->get(0);
}

void DLTDiff::setNode(fds_token_id token, uint index, NodeUuid nodeuuid) {
    std::map<fds_token_id, DltTokenGroupPtr>::const_iterator iter;
    iter = mapTokenNodes.find(token);
    if (iter != mapTokenNodes.end()) {
        iter->second->set(index, nodeuuid);
        return;
    }

    mapTokenNodes[token] = boost::shared_ptr<DltTokenGroup>
            (new DltTokenGroup(baseDlt->depth));
    mapTokenNodes[token]->set(index, nodeuuid);
}

//================================================================================
/**
 *  Implementation for DLT Manager
 */
DLTManager::DLTManager(fds_uint8_t maxDlts) : maxDlts(maxDlts) {
}

void DLTManager::checkSize() {
    // check if the no.of dlts stored are within limits
    if (dltList.size() > maxDlts) {
        DLT* pDlt = dltList.front();
        // make sure the top one is not the current One
        if (pDlt != curPtr) {
            // OK go ahead remove & delete
            dltList.erase(dltList.begin());
            delete pDlt;
        }
    }
}

// returns refcount of current DLT that was just replaced
Error DLTManager::add(const DLT& _newDlt,
                      FDS_Table::callback_type const& cb) {
    Error err(ERR_OK);
    DLT* pNewDlt = new DLT(_newDlt);
    DLT& newDlt = *pNewDlt;

    SCOPEDWRITE(dltLock);
    // check if we already have this DLT version in the list
    fds_uint64_t newDltVersion = pNewDlt->getVersion();
    for (std::vector<DLT*>::const_iterator iter = dltList.begin();
         iter != dltList.end();
         iter++) {
        if (newDltVersion == (*iter)->version) {
            LOGWARN << "DLT version " << newDltVersion
                    << " already exists in DLT manager, not going to add";
            return ERR_DUPLICATE;
        }
    }

    // check refcnt of the current DLT, if 0
    if (curPtr && (cb != nullptr)) {
        err = curPtr->setCallback(cb);
    }

    if (dltList.empty()) {
        dltList.push_back(pNewDlt);
        curPtr = pNewDlt;
        // TODO(prem): checkSize();
        return err;
    }

    const DLT& current = *curPtr;

    for (uint i = 0; i < newDlt.columns; i++) {
        if (current.distList->at(i) != newDlt.distList->at(i) && (current.depth == newDlt.depth)) { //NOLINT
            // There is the diff in pointer data
            // so check if there is a diff in actual data
            bool fSame = true;
            for (uint j = 0 ; j < newDlt.depth ; j++) {
                if (current.distList->at(i)->get(j) != newDlt.distList->at(i)->get(j)) {
                    fSame = false;
                    break;
                }
            }
            if (fSame) {
                // the contents are same
                newDlt.distList->at(i) = current.distList->at(i);
            } else {
                // Nothing to do for now.. We will leave it as is
            }
        }
    }
    dltList.push_back(&newDlt);
    // TODO(prem): checkSize();

    // switch this to current ???
    curPtr = &newDlt;

    return err;
}

Error DLTManager::addSerializedDLT(std::string& serializedData,
                                   FDS_Table::callback_type const& cb,
                                   bool fFull) { //NOLINT
    Error err(ERR_OK);
    DLT dlt(0, 0, 0, false);
    // Deserialize the DLT
    err = dlt.loadSerialized(serializedData);
    if (err.ok()) {
        // Recompute the node token map cache
        dlt.generateNodeTokenMap();
        err = add(dlt, cb);
    }
    return err;
}

bool DLTManager::add(const DLTDiff& dltDiff) {
    const DLT *baseDlt = dltDiff.baseDlt;
    if (!baseDlt) baseDlt=getDLT(dltDiff.baseVersion);
    LOGNOTIFY << "adding a diff - base:" << dltDiff.baseVersion
              << " version:" << dltDiff.version;

    DLT *dlt = new DLT(baseDlt->width, baseDlt->depth, dltDiff.version, false);
    dlt->distList->reserve(dlt->columns);

    DltTokenGroupPtr ptr;
    std::map<fds_token_id, DltTokenGroupPtr>::const_iterator iter;

    SCOPEDWRITE(dltLock);
    for (uint i = 0; i < dlt->columns; i++) {
        ptr = baseDlt->distList->at(i);

        iter = dltDiff.mapTokenNodes.find(i);
        if (iter != dltDiff.mapTokenNodes.end()) ptr = iter->second;

        if (curPtr->distList->at(i) != ptr) {
            bool fSame = true;
            for (uint j = 0 ; j < dlt->depth ; j++) {
                if (curPtr->distList->at(i)->get(j) != ptr->get(j)) {
                    fSame = false;
                    break;
                }
            }
            if (fSame) {
                ptr = curPtr->distList->at(i);
            }
        }
        dlt->distList->push_back(ptr);
    }

    dltList.push_back(dlt);

    return true;
}


const DLT* DLTManager::getDLT(const fds_uint64_t version) const {
    SCOPEDREAD(dltLock);
    if (0 == version) {
        return curPtr;
    }
    std::vector<DLT*>::const_iterator iter;
    for (iter = dltList.begin(); iter != dltList.end(); iter++) {
        if (version == (*iter)->version) {
            return *iter;
        }
    }

    return NULL;
}

const DLT* DLTManager::getAndLockCurrentVersion() {
    SCOPEDREAD(dltLock);
    if (curPtr != NULL) {
        fds_uint64_t refcnt = curPtr->incRefcnt();
        LOGDEBUG << "Current DLT version " << curPtr->getVersion()
                 << " refcnt " << refcnt;
    }
    return curPtr;
}

Error DLTManager::releaseVersion(fds_uint64_t version) {
    // version must be specified
    if (version == 0) {
        return ERR_INVALID_ARG;
    }
    std::vector<DLT*>::const_iterator iter;

    SCOPEDREAD(dltLock);
    for (iter = dltList.begin(); iter != dltList.end(); iter++) {
        if (version == (*iter)->version) {
            fds_uint64_t refcnt = (*iter)->decRefcnt();
            LOGDEBUG << "DLT version " << version << " refcnt " << refcnt;
            return ERR_OK;
        }
    }

    return ERR_NOT_FOUND;
}

std::vector<fds_uint64_t> DLTManager::getDltVersions() {
    std::vector<fds_uint64_t> vecVersions;
    std::vector<DLT*>::const_iterator iter;
    SCOPEDREAD(dltLock);
    for (iter = dltList.begin(); iter != dltList.end(); iter++) {
        vecVersions.push_back((*iter)->version);
    }
    return vecVersions;
}

Error DLTManager::setCurrent(fds_uint64_t version) {
    LOGNOTIFY << "Setting the current dlt to version: " << version;
    if (0 == version) {
        LOGNOTIFY << "Version 0 means current DLT, nothing to change";
        return ERR_OK;
    }
    std::vector<DLT*>::const_iterator iter;

    SCOPEDWRITE(dltLock);
    for (iter = dltList.begin(); iter != dltList.end(); ++iter) {
        if (version == (*iter)->version) {
            curPtr = *iter;
            return ERR_OK;
        }
    }

    // if we are here, we didn't find DLT with given version
    return ERR_NOT_FOUND;
}

Error DLTManager::setCurrentDltClosed() {
    if (curPtr == NULL) {
        LOGWARN << "No current DLT";
        return ERR_NOT_FOUND;
    }
    fds_uint64_t curVersion = curPtr->getVersion();

    // because curPtr is const pointer, we are searching again...
    std::vector<DLT*>::const_iterator iter;
    for (iter = dltList.begin(); iter != dltList.end(); ++iter) {
        if (curVersion == (*iter)->version) {
            LOGNOTIFY << "Setting current DLT (version " << curVersion
                      << ") to closed";
            (*iter)->setClosed();
            // Update curPtr since it's apparently not
            // done already.
            setCurrent(curVersion);
            break;
        }
    }
    return ERR_OK;
}

DltTokenGroupPtr DLTManager::getNodes(fds_token_id token) const {
    SCOPEDREAD(dltLock);
    return curPtr->getNodes(token);
}

DltTokenGroupPtr DLTManager::getNodes(const ObjectID& objId) const {
    SCOPEDREAD(dltLock);
    return curPtr->getNodes(objId);
}

// get the primary node for a token/objid
// NOTE:: from the current dlt!!!
NodeUuid DLTManager::getPrimary(fds_token_id token) const {
    SCOPEDREAD(dltLock);
    return curPtr->getPrimary(token);
}

NodeUuid DLTManager::getPrimary(const ObjectID& objId) const {
    return getPrimary(objId);
}

uint32_t DLTManager::write(serialize::Serializer*  s) const {
    LOGTRACE << " serializing dltmgr  ";
    uint32_t bytes = 0;
    // current version number
    bytes += s->writeI64(curPtr->version);

    // max no.of dlts
    bytes += s->writeByte(maxDlts);

    // number of dlts
    bytes += s->writeByte(dltList.size());

    // write the individual dlts
    std::vector<DLT*>::const_iterator iter;
    for (iter = dltList.begin(); iter != dltList.end(); iter++) {
        bytes += (*iter)->write(s);
    }

    return bytes;
}

uint32_t DLTManager::read(serialize::Deserializer* d) {
    LOGTRACE << " deserializing dltmgr  ";
    uint32_t bytes = 0;
    fds_uint64_t version;
    // current version number
    bytes += d->readI64(version);

    // max no.of dlts
    bytes += d->readByte(maxDlts);

    fds_uint8_t numDlts = 0;
    // number of dlts
    bytes += d->readByte(numDlts);

    // first clear the current dlts
    dltList.clear();

    // the individual dlts
    for (uint i = 0; i < numDlts ; i++) {
        DLT dlt(0, 0, 0, false);
        bytes += dlt.read(d);
        this->add(dlt);
    }

    // now set the current version
    setCurrent(version);

    return bytes;
}

bool DLTManager::loadFromFile(std::string filename) {
    LOGNOTIFY << "loading dltmgr from file : " << filename;
    serialize::Deserializer *d= serialize::getFileDeserializer(filename);
    read(d);
    delete d;
    return true;
}

bool DLTManager::storeToFile(std::string filename) {
    LOGNOTIFY << "storing dltmgr to file : " << filename;
    serialize::Serializer *s= serialize::getFileSerializer(filename);
    write(s);
    delete s;
    return true;
}

void DLTManager::dump() const {
    LOGNORMAL << "Dlt Manager : "
              << "[num.dlts : " << dltList.size() << "] "
              << "[maxDlts  : " << static_cast<int>(maxDlts) << "]";

    std::vector<DLT*>::const_iterator iter;

    for (iter = dltList.begin() ; iter != dltList.end() ; iter++) {
        (*iter)->dump();
    }
}

}  // namespace fds
