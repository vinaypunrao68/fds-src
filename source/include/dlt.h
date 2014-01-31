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
     * Class to store an array of nodeuuids that are part of a
     * DLT's token group. Had to do this for shared_ptr.
     * will consider shared_array later.
     */
    class DltTokenGroup {
  public:
        DltTokenGroup(NodeUuid uid0 = 0,
                      NodeUuid uid1 = 0,
                      NodeUuid uid2 = 0,
                      NodeUuid uid3 = 0) {
            p = new NodeUuid[MAX_REPL_FACTOR];
            p[0] = uid0;
            p[1] = uid1;
            p[2] = uid2;
            p[3] = uid3;
        }
        DltTokenGroup(const DltTokenGroup& n) {
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
        ~DltTokenGroup() {
            if (p) {
                delete[] p;
            }
        }

    private:
        NodeUuid* p;
    };

    typedef boost::shared_ptr<DltTokenGroup> DltTokenGroupPtr;
    typedef boost::shared_ptr<std::vector<DltTokenGroupPtr>> DistributionList;
    typedef std::vector<fds_token_id> TokenList;
    typedef std::map<NodeUuid, std::vector<fds_token_id>> NodeTokenMap;

    /**
     * Maintains the relation between a token and the responible nodes.
     * During lookup , an objid is converted to a token ..
     * Always generated the token using the getToken func.
     */
    class DLT {
    public :
        fds_uint64_t version;
        time_t timestamp;
        fds_uint32_t numTokens = MAX_TOKENS;
        fds_uint32_t width = MAX_REPL_FACTOR;

        /**
         * Get the Token for a given Object.
         * We need the first 16 bits
         */
        static fds_token_id getToken(const ObjectID& objId) {
            static uint TOKEN_BITMASK = ((1 << NUM_BITS_FOR_TOKENS) - 1);
            static uint BIT_OFFSET = (64 - NUM_BITS_FOR_TOKENS);
            return (fds_token_id)(TOKEN_BITMASK & (objId.GetHigh() >> BIT_OFFSET));
        }

        // fInit(true) will setup a blank dlt for adding the token ring info..
        // fInit(false) should be used to get the DLT from the DLTManager
        DLT(uint numTokens = MAX_TOKENS,
            uint width = MAX_REPL_FACTOR,
            bool fInit = false);
        DLT(const DLT& dlt);

        // get all the Nodes for a token/objid
        DltTokenGroupPtr getNodes(fds_token_id token) const;
        DltTokenGroupPtr getNodes(const ObjectID& objId) const;

        // get the primary node for a token/objid
        NodeUuid getPrimary(fds_token_id token) const;
        NodeUuid getPrimary(const ObjectID& objId) const;

        // get the Tokens for a given Node
        const TokenList& getTokens(NodeUuid uid) const;

        // set the node for given token at a given index
        // index range [0..MAX_REPLICA_FACTOR-1]
        // index[0] is the primary Node for that token
        void setNode(fds_token_id token, uint index, NodeUuid nodeuuid);
        void setNodes(fds_token_id token, const DltTokenGroup& nodes);

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
        DltTokenGroupPtr getNodes(fds_token_id token) const;
        DltTokenGroupPtr getNodes(const ObjectID& objId) const;

        // get the primary node for a token/objid
        // NOTE:: from the current dlt!!!
        NodeUuid getPrimary(fds_token_id token) const;
        NodeUuid getPrimary(const ObjectID& objId) const;

    private:
        DLT* curPtr;
        std::vector<DLT> dltList;
    };
}  // namespace fds
#endif  // SOURCE_INCLUDE_DLT_H_
