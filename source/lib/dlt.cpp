/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <dlt.h>
#include <vector>

/**
 *  Implementationg for DLT ...
 */

fds::DLT::DLT(uint numTokens, uint width, bool fInit) {
    this->numTokens = numTokens;
    this->width = width;

    distList = boost::shared_ptr<std::vector<NodeListPtr> >
            (new std::vector<NodeListPtr>());
    if (fInit) {
        distList->reserve(numTokens);
        for (uint i = 0; i < numTokens; i++) {
            distList->push_back(boost::shared_ptr<NodeList>(new NodeList()));
        }
    }
}

fds::DLT::DLT(const DLT& dlt) {
    numTokens = dlt.numTokens;
    width = dlt.width;
    distList = dlt.distList;
    mapNodeTokens = dlt.mapNodeTokens;
}

// get all the Nodes for a token/objid
fds::NodeListPtr fds::DLT::getNodes(Token token) const {
    fds_verify(token < numTokens);
    return distList->at(token);
}

fds::NodeListPtr fds::DLT::getNodes(const ObjectID& objId) const {
    Token token = getToken(objId);
    fds_verify(token < numTokens);
    return distList->at(token);
}

// get the primary node for a token/objid
fds::NodeUuid fds::DLT::getPrimary(Token token) const {
    fds_verify(token < numTokens);
    return getNodes(token)->get(0);
}

fds::NodeUuid fds::DLT::getPrimary(const ObjectID& objId) const {
    return getNodes(objId)->get(0);
}

void fds::DLT::setNode(Token token, uint index, NodeUuid nodeuuid) {
    fds_verify(index < MAX_REPL_FACTOR);
    fds_verify(token < numTokens);
    distList->at(token)->set(index, nodeuuid);
}

void fds::DLT::setNodes(Token token, const NodeList& nodes) {
    for (uint i = 0; i < MAX_REPL_FACTOR ; i++) {
        distList->at(token)->set(i, nodes.get(i));
    }
}

void fds::DLT::generateNodeTokenMap() {
    std::vector<NodeListPtr>::const_iterator iter;
    mapNodeTokens->clear();
    uint i;
    Token token = 0;
    for (iter = distList->begin(); iter != distList->end(); iter++) {
        const NodeListPtr& nodeList= *iter;
        for (i = 0; i < MAX_REPL_FACTOR ; i++){
            (*mapNodeTokens)[nodeList->get(i)].push_back(token);
        }
        token++;
    }
}

// get the Tokens for a given Node
const fds::TokenList& fds::DLT::getTokens(NodeUuid uid) const{
    static TokenList emptyTokenList;
    NodeTokenMap::const_iterator iter = mapNodeTokens->find(uid);
    if (iter != mapNodeTokens->end()) {
        return iter->second;
    } else {
        // TODO(prem) : need to revisit this
        return  emptyTokenList;
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

