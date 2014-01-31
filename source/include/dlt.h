/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_DLT_H_
#define SOURCE_INCLUDE_DLT_H_

#include <vector>
#include <map>
#include <list>

#include <boost/shared_ptr.hpp>

#include <fds_typedefs.h>
#include <fds_defines.h>
#include <fds_module.h>

namespace fds {
    /**
     * Class to store an array of nodeuuids that are part of a
     * DLT's token group. Had to do this for shared_ptr.
     * will consider shared_array later.
     */
    class DltTokenGroup {
  public:
        explicit DltTokenGroup(fds_uint32_t len)
                : length(len) {
            // Create an array of uuid set to 0
            p = new NodeUuid[length]();
        }
        explicit DltTokenGroup(const DltTokenGroup& n) {
            length = n.length;
            p = new NodeUuid[length];
            // TODO(Prem): change to memcpy later
            for (fds_uint32_t i = 0; i < length; i++) {
                p[i] = n.p[i];
            }
        }
        void set(fds_uint32_t index, NodeUuid uid) {
            fds_verify(index <= length);
            p[index] = uid;
        }
        NodeUuid get(uint index) const {
            fds_verify(index <= length);
            return p[index];
        }

        NodeUuid& operator[] (int index) {
            return p[index];
        }

        fds_uint32_t getLength() const {
            return length;
        }
        ~DltTokenGroup() {
            if (p) {
                delete[] p;
            }
        }

  private:
        NodeUuid     *p;
        fds_uint32_t length;
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
    class DLT : public Module {
  public :
        /**
         * Return the token ID for a given Object based on
         * the size of this DLT.
         */
        fds_token_id getToken(const ObjectID& objId) const;

        /**
         * fInit(true) will setup a blank dlt for adding the token ring info..
         * fInit(false) should be used to get the DLT from the DLTManager
         */
        DLT(fds_uint32_t _width,
            fds_uint32_t _depth,
            fds_uint64_t _version,
            bool fInit = false);

        // This is a shallow copy - just for convenience.
        // if you change a copy's TokenGroup data , the original will be changed
        // Mebbe we can remove this later
        DLT(const DLT& dlt);

        // Deep copy . Safe modifiable copy.
        DLT& clone();

        /** get all the Nodes for a token/objid */
        DltTokenGroupPtr getNodes(fds_token_id token) const;
        DltTokenGroupPtr getNodes(const ObjectID& objId) const;

        /** get the primary node for a token/objid */
        NodeUuid getPrimary(fds_token_id token) const;
        NodeUuid getPrimary(const ObjectID& objId) const;

        /** get the Tokens for a given Node */
        const TokenList& getTokens(const NodeUuid &uid) const;

        /**
         * set the node for given token at a given index
         * index range [0..MAX_REPLICA_FACTOR-1]
         * index[0] is the primary Node for that token
         */
        void setNode(fds_token_id token, uint index, NodeUuid nodeuuid);
        void setNodes(fds_token_id token, const DltTokenGroup& nodes);

        /**
         * generate the NodeUUID to TokenList Map.
         * Need to be called only when the DLT is created from scratch .
         * after the data is loaded..
         */
        void generateNodeTokenMap();

        fds_uint64_t getVersion() const;  /**< Gets version */
        fds_uint32_t getDepth() const;  /**< Gets num replicas per token */
        fds_uint32_t getWidth() const;  /**< Gets num bits used */
        fds_uint32_t getNumTokens() const;  /** Gets total num of tokens */

        /*
         * Module members
         */
        int  mod_init(fds::SysParams const *const param);
        void mod_startup();
        void mod_shutdown();

  private:
        /**
         * Maps token ids to the group of nodes.
         * Constitutes main dlt structure.
         */
        DistributionList distList;

        /** Cached reverse map from node to its token ids */
        boost::shared_ptr<NodeTokenMap> mapNodeTokens;
        friend class DLTManager;
        friend class DLTDiff;

        fds_uint64_t version;    /**< OM DLT version */
        time_t       timestamp;  /**< Time OM created DLT */
        fds_uint32_t width;      /**< Specified as 2^width */
        fds_uint32_t numTokens;  /**< Expanded version of width */
        fds_uint32_t depth;      /**< Depth of each token group */
    };

    /**
     * This class maintains the diff between a given dlt and another
     * Modification to the data will be stored separately without
     * affecting the original dlt. A DLTDiff can be added to the
     * DLTManager to be stored as a new DLT. Hopefully only the DLTDiff
     * will be transmitted across.
     */
    class DLTDiff {
  public:
        // dlt : is the base dlt relative to which diffs will be maintained
        // version : is the new version number of the dltdiff. if version is 0
        // the version will be set as dlt.version1
        DLTDiff(DLT* baseDlt, fds_uint64_t version = 0);

        // get all the Nodes for a token/objid
        DltTokenGroupPtr getNodes(fds_token_id token) const;
        DltTokenGroupPtr getNodes(const ObjectID& objId) const;

        // get the primary node for a token/objid
        NodeUuid getPrimary(fds_token_id token) const;
        NodeUuid getPrimary(const ObjectID& objId) const;

        void setNode(fds_token_id token, uint index, NodeUuid nodeuuid);

        TokenList& getChangedTokens();

        fds_uint64_t version;
        time_t timestamp;

  private:
        bool fNewDlt;
        DLT* baseDlt;
        fds_uint64_t baseVersion;
        std::map<fds_token_id, DltTokenGroupPtr> mapTokenNodes;
        friend class DLTManager;
    };

    /**
     * Manages multiple DLTs ,maintains the current DLT
     * Generates the diff between dlts..
     */
    typedef boost::shared_ptr<const DLT> DLTPtr;

    class DLTManager {
  public :
        DLTManager();

        bool add(const DLT& dlt);
        bool add(const DLTDiff& dltDiff);

        // By default the get the current one(0) or the specific version
        const DLT* getDLT(fds_uint64_t version = 0);

        // Make the specific version as the current
        void setCurrent(fds_uint64_t version);

        // get all the Nodes for a token/objid
        // NOTE:: from the current dlt!!!
        DltTokenGroupPtr getNodes(fds_token_id token) const;
        DltTokenGroupPtr getNodes(const ObjectID& objId) const;

        // get the primary node for a token/objid
        // NOTE:: from the current dlt!!!
        NodeUuid getPrimary(fds_token_id token) const;
        NodeUuid getPrimary(const ObjectID& objId) const;

  private:
        const DLT* curPtr;
        std::vector<DLT> dltList;
    };
}  // namespace fds
#endif  // SOURCE_INCLUDE_DLT_H_
