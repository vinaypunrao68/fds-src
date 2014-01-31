/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_DLT_H_
#define SOURCE_INCLUDE_DLT_H_

#include <vector>
#include <map>

#include <boost/shared_ptr.hpp>

#include <fds_typedefs.h>
#include <fds_defines.h>

namespace fds {
    /**
     * Struct to store an array of nodeuuid. had to do this for shared_ptr.
     * will consider shared_array later.
     */
    struct NodeList {
        NodeList(NodeUuid uid0 = 0,
                 NodeUuid uid1 = 0,
                 NodeUuid uid2 = 0,
                 NodeUuid uid3 = 0) {
            p = new NodeUuid[MAX_REPL_FACTOR];
            p[0] = uid0;
            p[1] = uid1;
            p[2] = uid2;
            p[3] = uid3;
        }
        NodeList(const NodeList& n) {
            p = new NodeUuid[MAX_REPL_FACTOR];
            // TODO(Prem): change to memcpy later
            for (uint i = 0; i < MAX_REPL_FACTOR; i++) {
                p[i] = n.p[i];
            }
        }
        void set(uint index, NodeUuid uid) {
            p[index] = uid;
        }
        NodeUuid get(uint index) const {
            return p[index];
        }
        ~NodeList() {
            if (p) {
                delete[] p;
            }
        }

    private:
        NodeUuid* p;
    };

    typedef fds_uint16_t Token;
    typedef boost::shared_ptr<NodeList> NodeListPtr;
    typedef boost::shared_ptr<std::vector<NodeListPtr>>  DistributionList;
    typedef std::vector<Token> TokenList;
    typedef std::map<NodeUuid, std::vector<Token>> NodeTokenMap;

    /**
     * Maintains the relation between a token and the responible nodes.
     * During lookup , an objid is converted to a token ..
     * Always generated the token using the getToken func.
     */
    class DLT {
    public :
        uint version;
        time_t timestamp;
        uint numTokens = MAX_TOKENS;
        uint width = MAX_REPL_FACTOR;

        /**
         * Get the Token for a given Object.
         * We need the first 16 bits
         */
        static Token getToken(const ObjectID& objId) {
            static uint TOKEN_BITMASK = ((1 << NUM_BITS_FOR_TOKENS) - 1);
            static uint BIT_OFFSET = (64 - NUM_BITS_FOR_TOKENS);
            return (Token)(TOKEN_BITMASK & (objId.GetHigh() >> BIT_OFFSET));
        }

        // fInit(true) will setup a blank dlt for adding the token ring info..
        // fInit(false) should be used to get the DLT from the DLTManager
        DLT(uint numTokens = MAX_TOKENS,
            uint width = MAX_REPL_FACTOR,
            bool fInit = false);
        DLT(const DLT& dlt);

        // get all the Nodes for a token/objid
        NodeListPtr getNodes(Token token) const;
        NodeListPtr getNodes(const ObjectID& objId) const;

        // get the primary node for a token/objid
        NodeUuid getPrimary(Token token) const;
        NodeUuid getPrimary(const ObjectID& objId) const;

        // get the Tokens for a given Node
        const TokenList& getTokens(NodeUuid uid) const;

        // set the node for given token at a given index
        // index range [0..MAX_REPLICA_FACTOR-1]
        // index[0] is the primary Node for that token
        void setNode(Token token, uint index, NodeUuid nodeuuid);
        void setNodes(Token token, const NodeList& nodes);

        // generate the NodeUUID to TokenList Map.
        // Need to be called only when the DLT is created from scratch .
        // after the data is loaded..

        void generateNodeTokenMap();

    private:
        DistributionList distList;

        boost::shared_ptr<NodeTokenMap> mapNodeTokens;
        friend class DLTManager;
    };

    /**
     * Manages multiple DLTs ,maintains the current DLT
     * Generates the diff between dlts..
     */
    typedef boost::shared_ptr<const DLT> DLTPtr;

    class DLTManager {
    public :
        DLTManager()
                : curPtr(NULL) {
        }

        bool add(const DLT& dlt);

        // By default the get the current one(0) or the specific version
        const DLT* getDLT(uint version = 0);

        // Make the specific version as the current
        void setCurrent(uint version);

        // get all the Nodes for a token/objid
        // NOTE:: from the current dlt!!!
        NodeListPtr getNodes(Token token) const;
        NodeListPtr getNodes(const ObjectID& objId) const;

        // get the primary node for a token/objid
        // NOTE:: from the current dlt!!!
        NodeUuid getPrimary(Token token) const;
        NodeUuid getPrimary(const ObjectID& objId) const;

    private:
        DLT* curPtr;
        std::vector<DLT> dltList;
    };
}  // namespace fds
#endif  // SOURCE_INCLUDE_DLT_H_
