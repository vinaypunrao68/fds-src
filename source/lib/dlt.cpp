/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <cmath>
#include <vector>
#include <map>
#include <dlt.h>

/**
 *  Implementationg for DLT ...
 */
namespace fds {

DLT::DLT(fds_uint32_t _width,
         fds_uint32_t _depth,
         fds_uint64_t _version,
         bool fInit)
        : Module("data placement table"),
          width(_width), depth(_depth),
          numTokens(pow(2, _width)),
          version(_version) {
    distList = boost::shared_ptr<std::vector<DltTokenGroupPtr> >
            (new std::vector<DltTokenGroupPtr>());

    // Pre-allocate token groups for each token
    if (fInit) {
        distList->reserve(numTokens);
        for (uint i = 0; i < numTokens; i++) {
            distList->push_back(boost::shared_ptr<DltTokenGroup>(
                new DltTokenGroup(depth)));
        }
    }
}

DLT::DLT(const DLT& dlt)
        : Module("data placement table"),
          width(dlt.width),
          depth(dlt.depth),
          version(dlt.version),
          distList(dlt.distList),
          mapNodeTokens(dlt.mapNodeTokens) {
}

DLT& DLT::clone() {
    DLT *pDlt = new DLT(numTokens, width, true);
    pDlt->version = version;
    pDlt->timestamp = timestamp;

    for (uint i = 0; i < numTokens; i) {
        for (uint j = 0 ; j < width ; j) {
            pDlt->distList->at(i)->set(j, distList->at(i)->get(j));
        }
    }
    return *pDlt;
}

int DLT::mod_init(SysParams const *const param) {
    Module::mod_init(param);
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

fds_uint32_t
DLT::getWidth() const {
    return width;
}

fds_uint32_t
DLT::getNumTokens() const {
    return numTokens;
}

fds_token_id DLT::getToken(const ObjectID& objId) const {
    fds_uint64_t token_bitmask = ((1 << width) - 1);
    fds_uint64_t bit_offset = (sizeof(objId.GetHigh()) - width);
    return (fds_token_id)(token_bitmask & (objId.GetHigh() >> bit_offset));
}

// get all the Nodes for a token/objid
DltTokenGroupPtr DLT::getNodes(fds_token_id token) const {
    fds_verify(token < numTokens);
    return distList->at(token);
}

DltTokenGroupPtr DLT::getNodes(const ObjectID& objId) const {
    fds_token_id token = getToken(objId);
    fds_verify(token < numTokens);
    return distList->at(token);
}

// get the primary node for a token/objid
NodeUuid DLT::getPrimary(fds_token_id token) const {
    fds_verify(token < numTokens);
    return getNodes(token)->get(0);
}

NodeUuid DLT::getPrimary(const ObjectID& objId) const {
    return getNodes(objId)->get(0);
}

void DLT::setNode(fds_token_id token, uint index, NodeUuid nodeuuid) {
    fds_verify(index < depth);
    fds_verify(token < numTokens);
    distList->at(token)->set(index, nodeuuid);
}

void DLT::setNodes(fds_token_id token, const DltTokenGroup& nodes) {
    fds_verify(nodes.getLength() == depth);
    for (uint i = 0; i < depth ; i++) {
        distList->at(token)->set(i, nodes.get(i));
    }
}

void DLT::generateNodeTokenMap() {
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
const TokenList& DLT::getTokens(const NodeUuid &uid) const{
    static TokenList emptyTokenList;
    NodeTokenMap::const_iterator iter = mapNodeTokens->find(uid);
    if (iter != mapNodeTokens->end()) {
        return iter->second;
    } else {
        // TODO(prem) : need to revisit this
        return emptyTokenList;
    }
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
bool DLTManager::add(const DLT& _newDlt) {
    DLT* pNewDlt = new DLT(_newDlt);
    DLT& newDlt = *pNewDlt;

    if (dltList.empty()) {
        dltList.push_back(newDlt);
        curPtr = pNewDlt;
        return true;
    }

    const DLT& current = *curPtr;

    for (uint i = 0; i < newDlt.numTokens; i++) {
        if (current.distList->at(i) != newDlt.distList->at(i)) {
            // There is the diff in pointer data
            // so check if there is a diff in actual data
            bool fSame = true;
            for (uint j = 0 ; j < newDlt.width ; j++) {
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
    dltList.push_back(newDlt);

    // switch this to current ???
    curPtr = &newDlt;

    return true;
}

bool DLTManager::add(const DLTDiff& dltDiff) {
    const DLT *baseDlt = dltDiff.baseDlt;
    if (!baseDlt) baseDlt=getDLT(dltDiff.baseVersion);

    DLT *dlt = new DLT(baseDlt->numTokens, baseDlt->width, false);
    dlt->distList->reserve(dlt->numTokens);

    DltTokenGroupPtr ptr;
    std::map<fds_token_id, DltTokenGroupPtr>::const_iterator iter;

    for (uint i = 0; i < dlt->numTokens; i) {
        ptr = baseDlt->distList->at(i);

        iter = dltDiff.mapTokenNodes.find(i);
        if (iter != dltDiff.mapTokenNodes.end()) ptr = iter->second;

        if (curPtr->distList->at(i) != ptr) {
            bool fSame = true;
            for (uint j = 0 ; j < dlt->width ; j) {
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

    dltList.push_back(*dlt);

    return true;
}


const DLT* DLTManager::getDLT(const fds_uint64_t version) {
    if (0 == version) {
        return curPtr;
    }
    std::vector<DLT>::const_iterator iter;
    for (iter = dltList.begin(); iter != dltList.end(); iter++) {
        if (version == iter->version) {
            return &(*iter);
        }
    }

    return NULL;
}


void DLTManager::setCurrent(fds_uint64_t version) {
    std::vector<DLT>::const_iterator iter;
    for (iter = dltList.begin(); iter != dltList.end(); iter++) {
        if (version == iter->version) {
            curPtr= &(*iter);
            break;
        }
    }
}

DltTokenGroupPtr DLTManager::getNodes(fds_token_id token) const {
    return curPtr->getNodes(token);
}

DltTokenGroupPtr DLTManager::getNodes(const ObjectID& objId) const {
    return curPtr->getNodes(objId);
}

// get the primary node for a token/objid
// NOTE:: from the current dlt!!!
NodeUuid DLTManager::getPrimary(fds_token_id token) const {
    return curPtr->getPrimary(token);
}

NodeUuid DLTManager::getPrimary(const ObjectID& objId) const {
    return getPrimary(objId);
}

}  // namespace fds
