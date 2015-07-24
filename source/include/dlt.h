/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_DLT_H_
#define SOURCE_INCLUDE_DLT_H_

#include <vector>
#include <map>
#include <mutex>
#include <list>
#include <set>

#include <boost/shared_ptr.hpp>

#include <fds_resource.h>
#include <fds_typedefs.h>
#include <fds_defines.h>
#include <fds_module.h>
#include <serialize.h>
#include <string>
#include <ostream>
#include <util/Log.h>
#include <util/timeutils.h>
#include <fds_placement_table.h>
#include <concurrency/RwLock.h>

namespace fds {
#define DLT_VER_INVALID 0UL  /**< Defines 0 as invalid DLT version */
    const fds_uint8_t sm1Idx = 0;  /* DLT slot identifying SM1. */

    typedef TableColumn DltTokenGroup;
    typedef boost::shared_ptr<DltTokenGroup> DltTokenGroupPtr;
    typedef boost::shared_ptr<std::vector<DltTokenGroupPtr>> DistributionList;
    typedef std::vector<fds_token_id> TokenList;
    typedef std::map<NodeUuid, std::vector<fds_token_id>> NodeTokenMap;
    /**
     * Callback for DLT update response
     */
    typedef std::function<void (const Error&)> OmDltUpdateRespCbType;

    /**
     * Maintains the relation between a token and the responible nodes.
     * During lookup , an objid is converted to a token ..
     * Always generated the token using the getToken func.
     */
    struct DLT :  serialize::Serializable , public Module , HasLogger {
  public :
        /**
         * Return the token ID for a given Object based on
         * the size of this DLT.
         */
        fds_token_id getToken(const ObjectID& objId) const;
        static fds_token_id getToken(const ObjectID& objId,
                                     fds_uint32_t bitsPerToken);

        /**
         * fInit(true) will setup a blank dlt for adding the token ring info..
         * fInit(false) should be used to get the DLT from the DLTManager
         */
        DLT(fds_uint32_t numBitsForToken,
            fds_uint32_t depth,
            fds_uint64_t version,
            bool fInit = false);

        // This is a shallow copy - just for convenience.
        // if you change a copy's TokenGroup data , the original will be changed
        // Mebbe we can remove this later
        DLT(const DLT& dlt);
        ~DLT();

        // Deep copy . Safe modifiable copy.
        DLT* clone() const;

        /** get all the Nodes for a token/objid */
        DltTokenGroupPtr getNodes(fds_token_id token) const;
        DltTokenGroupPtr getNodes(const ObjectID& objId) const;

        /** get the primary node for a token/objid */
        NodeUuid getPrimary(fds_token_id token) const;
        NodeUuid getPrimary(const ObjectID& objId) const;

        /** get the node in the specified index*/
        NodeUuid getNode(fds_token_id token, uint index) const;
        NodeUuid getNode(const ObjectID& objId, uint index) const;

        int getIndex(const fds_token_id& token, const NodeUuid& nodeUuid) const;
        int getIndex(const ObjectID& objId, const NodeUuid& nodeUuid) const;

        /** get the Tokens for a given Node */
        const TokenList& getTokens(const NodeUuid &uid) const;
        void getTokens(TokenList* tokenList, const NodeUuid &uid, uint index) const;

        typedef std::map<NodeUuid, std::vector<fds_int32_t>> SourceNodeMap;

        /**
         * To resync token data from a node in the redundancy group,
         * get source node for a given token. The only choice for the
         * source node will be the node in slot 0 (SM1) for a given token.
         * If primary is not available, then token migration can not take place.
         */
        NodeUuid getSourceNodeForToken(const NodeUuid &destNodeUuid,
                                       const fds_token_id &tokenId) const;

        /**
         * Get source nodes for all the tokens of a given node. This
         * fills up a passed map of <Source Node Uuid - std::vector of tokens>.
         */
        void getSourceForAllNodeTokens(const NodeUuid &nodeUuid,
                                       SourceNodeMap &srcNodeMap) const;

        /**
         * Get source SM(s) for a given <current source SM - dlt tokens>
         * NodeTokenMap[INVALID_RESOURCE_UUID] holds all dlt tokens for
         * whom no source SM could be assigned.
         */
        NodeTokenMap getNewSourceSMs(const NodeUuid& curSrcSM,
                                     const std::set<fds_token_id>& dlt_tokens,
                                     const uint8_t& retryCount,
                                     std::map<NodeUuid, bool>& failedSMs) const;

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
        void generateNodeTokenMap() const;

        fds_uint64_t getVersion() const;  /**< Gets version */
        fds_uint32_t getDepth() const;  /**< Gets num replicas per token */
        fds_uint32_t getWidth() const;  /**< Gets num bits used */
        fds_uint32_t getNumBitsForToken() const;  /**< Gets num bits used */
        fds_uint32_t getNumTokens() const;  /** Gets total num of tokens */
        fds_bool_t isClosed() const;

        void setClosed();
        /**
         * Increments refcount and returns current refcount
         */
        fds_uint64_t incRefcnt();
        /**
         * Decrements refcount and returns current refcount
         */
        fds_uint64_t decRefcnt();
        fds_uint64_t getRefcnt();
        /**
         * @return ERR_DLT_IO_PENDING if refcount > 0 and cb was stored;
         * otherwise ERR_OK
         */
        Error addCbIfRefcnt(OmDltUpdateRespCbType cb);

        void getTokenObjectRange(const fds_token_id &token,
                ObjectID &begin, ObjectID &end) const;

        uint32_t virtual write(serialize::Serializer*  s) const;
        uint32_t virtual read(serialize::Deserializer* d);

        bool loadFromFile(std::string filename);
        bool storeToFile(std::string filename);

        uint32_t getEstimatedSize() const;

        /**
         * Two DLTs are equal if they have the same content in the table
         * but versions may be different (or same).
         */
        fds_bool_t operator==(const DLT &rhs) const;

        // print the dlt to the logs
        // will print full dlt if the loglevel is debug
        void dump() const;

        /**
         * Checks if DLT is valid
         * Invalid cases:
         *    -- A column has repeating node uuids (non-unique)
         *    -- A cell in a DMT has an invalid Service UUID
         *    -- DLT must not contain any uuids that are not in 'expectedUuidSet'
         * @param expectedUuidSet a set of UUIDs that are expected to be
         *        in this DLT; one or more UUIDs may be missing from the DLT,
         *        but DLT must not contain any UUID that is not in the set
         * @return ERR_OK or ERR_DLT_INVALID
         */
        Error verify(const NodeUuidSet& expectedUuidSet) const;

        /*
         * Module members
         */
        int  mod_init(fds::SysParams const *const param);
        void mod_startup();
        void mod_shutdown();

        /*
         * Static methods
         */
        static std::set<fds_token_id> token_diff(const NodeUuid &uid,
                const DLT* new_dlt, const DLT* old_dlt);

        friend std::ostream& operator<< (std::ostream &out, const DLT& dlt);

  private:
        /**
         * Maps token ids to the group of nodes.
         * Constitutes main dlt structure.
         */
        DistributionList distList;

        /** Cached reverse map from node to its token ids */
        mutable std::once_flag mapInitialized;
        mutable boost::shared_ptr<NodeTokenMap> mapNodeTokens;
        friend class DLTManager;
        friend class DLTDiff;

        fds_uint64_t version;    /**< OM DLT version */
        util::TimeStamp    timestamp;  /**< Time OM created DLT */
        fds_bool_t   closed;     /**< true if DLT is closed, not only commited */
        fds_uint32_t numBitsForToken;      /**< numTokens = 2^numBitsForToken */
        fds_uint32_t numTokens;  /**< Expanded version of width */
        fds_uint32_t depth;      /**< Depth of each token group */

        // refcount DLT accesses
        std::atomic<fds_uint64_t> refcnt;
        // if callback is set, will be called when refcnt becomes 0
        OmDltUpdateRespCbType omDltUpdateCb;
    };

    /**
     * This class maintains the diff between a given dlt and another
     * Modification to the data will be stored separately without
     * affecting the original dlt. A DLTDiff can be added to the
     * DLTManager to be stored as a new DLT. Hopefully only the DLTDiff
     * will be transmitted across.
     */
    struct DLTDiff {
        // dlt : is the base dlt relative to which diffs will be maintained
        // version : is the new version number of the dltdiff. if version is 0
        // the version will be set as dlt.version1
        explicit DLTDiff(DLT* baseDlt, fds_uint64_t version = 0);

        // get all the Nodes for a token/objid
        DltTokenGroupPtr getNodes(fds_token_id token) const;
        DltTokenGroupPtr getNodes(const ObjectID& objId) const;

        // get the primary node for a token/objid
        NodeUuid getPrimary(fds_token_id token) const;
        NodeUuid getPrimary(const ObjectID& objId) const;

        void setNode(fds_token_id token, uint index, NodeUuid nodeuuid);

        TokenList& getChangedTokens();

        fds_uint64_t version;
        util::TimeStamp timestamp;

  private:
        bool fNewDlt;
        DLT* baseDlt;
        fds_uint64_t baseVersion;
        std::map<fds_token_id, DltTokenGroupPtr> mapTokenNodes;
        friend class DLTManager;
    };
    typedef boost::shared_ptr<DLT> DLTPtr;

    /**
     * Manages multiple DLTs ,maintains the current DLT
     * Generates the diff between dlts..
     */
    struct DLTManager :  HasLogger, serialize::Serializable {
        explicit DLTManager(fds_uint8_t maxDlts = 2);

        /**
         * If cb is provided and returned error = ERR_OK, then provided
         * callback will be called when refcnt becomes 0
         * @return If cb is provided: ERR_DLT_IO_PENDING if DLT was added successfully,
         * but previous DLT has refcount > 0; otherwise ERR_OK if DLT was added successfully
         */
        Error add(const DLT& dlt,
                  OmDltUpdateRespCbType cb = nullptr);
        bool add(const DLTDiff& dltDiff);
        /**
         * Adds DLT to the list and makes this DLT "committed" (current)
         * @return ERR_DLT_IO_PENDING if DLT was added successfully, but
         * previous DLT has refcount > 0; ERR_OK if DLT was added successfully
         * and previous DLT refcount == 0
         */
        Error addSerializedDLT(std::string& serializedData,
                               OmDltUpdateRespCbType cb,
                               bool fFull = true);  // NOLINT

        // By default the get the current one(0) or the specific version
        const DLT* getDLT(fds_uint64_t version = 0) const;
        /**
         * Returns the current DLT and increments refcount for DLT accesses
         */
        const DLT* getAndLockCurrentDLT();
        /**
         * Decrements refcount for given DLT version
         * @return ERR_INVALID_ARG if version is 0
         */
        Error decDLTRefcnt(fds_uint64_t version);

        std::vector<fds_uint64_t> getDltVersions();

        // Make the specific version as the current
        Error setCurrent(fds_uint64_t version);
        Error setCurrentDltClosed();

        // get all the Nodes for a token/objid
        // NOTE:: from the current dlt!!!
        DltTokenGroupPtr getNodes(fds_token_id token) const;
        DltTokenGroupPtr getNodes(const ObjectID& objId) const;

        // get the primary node for a token/objid
        // NOTE:: from the current dlt!!!
        NodeUuid getPrimary(fds_token_id token) const;
        NodeUuid getPrimary(const ObjectID& objId) const;

        uint32_t virtual write(serialize::Serializer*  s) const;
        uint32_t virtual read(serialize::Deserializer* d);

        bool loadFromFile(std::string filename);
        bool storeToFile(std::string filename);

        // print the dlt manager to the logs
        // will print full dlt if the loglevel is debug
        void dump() const;

  private:
        DLT* curPtr = NULL;
        std::vector<DLT*> dltList;
        fds_rwlock mutable dltLock;  /**< lock protecting curPtr and dltList */
        void checkSize();
        fds_uint8_t maxDlts;
    };

    typedef boost::shared_ptr<DLTManager> DLTManagerPtr;

}  // namespace fds
#endif  // SOURCE_INCLUDE_DLT_H_
