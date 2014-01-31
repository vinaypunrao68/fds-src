/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <cmath>
#include <vector>

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
          numTokens(pow(2, _width) - 1),
          version(_version) {
    distList = boost::shared_ptr<std::vector<DltTokenGroupPtr> >
            (new std::vector<DltTokenGroupPtr>());

    // Pre-allocate token groups for each token
    if (fInit) {
        distList->reserve(numTokens);
        for (uint i = 0; i < numTokens; i++) {
            distList->push_back(boost::shared_ptr<DltTokenGroup>(
                new DltTokenGroup(numTokens)));
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

int
DLT::mod_init(SysParams const *const param) {
    Module::mod_init(param);
}

void
DLT::mod_startup() {
}

void
DLT::mod_shutdown() {
}

fds_token_id
DLT::getToken(const ObjectID& objId) const {
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


/**
 *  Implementation for DLT Manager
 */
bool fds::DLTManager::add(const DLT& _newDlt) {
    DLT* pNewDlt = new DLT(_newDlt);
    DLT& newDlt = *pNewDlt;

    if (dltList.empty()) {
        dltList.push_back(newDlt);
        curPtr = pNewDlt;
        return true;
    }

    DLT& current = *curPtr;

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

const fds::DLT* fds::DLTManager::getDLT(const uint version) {
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

}  // namespace fds
