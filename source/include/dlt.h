/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_DLT_H_
#define SOURCE_INCLUDE_DLT_H_

#include <vector>
#include <map>
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

namespace fds {
#define DLT_VER_INVALID 0  /**< Defines 0 as invalid DLT version */

    typedef TableColumn DltTokenGroup;
    typedef boost::shared_ptr<DltTokenGroup> DltTokenGroupPtr;
    typedef boost::shared_ptr<std::vector<DltTokenGroupPtr>> DistributionList;
    typedef std::vector<fds_token_id> TokenList;
    typedef std::map<NodeUuid, std::vector<fds_token_id>> NodeTokenMap;

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

        void getTokenObjectRange(const fds_token_id &token,
                ObjectID &begin, ObjectID &end) const;

        uint32_t virtual write(serialize::Serializer*  s) const;
        uint32_t virtual read(serialize::Deserializer* d);

        uint32_t getEstimatedSize() const;

        // print the dlt to the logs
        // will print full dlt if the loglevel is debug
        void dump() const;

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
        mutable boost::shared_ptr<NodeTokenMap> mapNodeTokens;
        friend class DLTManager;
        friend class DLTDiff;

        fds_uint64_t version;    /**< OM DLT version */
        util::TimeStamp    timestamp;  /**< Time OM created DLT */
        fds_uint32_t numBitsForToken;      /**< numTokens = 2^numBitsForToken */
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
    struct DLTDiff {
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

        bool add(const DLT& dlt);
        bool add(const DLTDiff& dltDiff);
        Error addSerializedDLT(std::string& serializedData, bool fFull = true);  // NOLINT

        // By default the get the current one(0) or the specific version
        const DLT* getDLT(fds_uint64_t version = 0) const;

        std::vector<fds_uint64_t> getDltVersions() const;

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

        uint32_t virtual write(serialize::Serializer*  s) const;
        uint32_t virtual read(serialize::Deserializer* d);

        bool loadFromFile(std::string filename);
        bool storeToFile(std::string filename);

        // print the dlt manager to the logs
        // will print full dlt if the loglevel is debug
        void dump() const;

  private:
        const DLT* curPtr = NULL;
        std::vector<DLT*> dltList;
        void checkSize();
        fds_uint8_t maxDlts;
    };

    typedef boost::shared_ptr<DLTManager> DLTManagerPtr;

}  // namespace fds
#endif  // SOURCE_INCLUDE_DLT_H_
