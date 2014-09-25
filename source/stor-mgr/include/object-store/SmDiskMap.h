/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMDISKMAP_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMDISKMAP_H_

#include <string>
#include <set>

#include <fds_module.h>
#include <dlt.h>
#include <persistent_layer/dm_io.h>
#include <object-store/SmTokenPlacement.h>

namespace fds {

/*
 * SmDiskMap keeps track of data and metadata SM token layout
 * on disks, and manages SM tokens persistent state.
 */
class SmDiskMap : public Module, public boost::noncopyable {
  public:
    explicit SmDiskMap(const std::string& modName);
    ~SmDiskMap();

    typedef std::unique_ptr<SmDiskMap> unique_ptr;

    /**
     * Updates SM token on-disk location table.
     * Currently assumes that new DLT does not change ownership
     * of SM tokens by this SM, will assert otherwise. This will
     * change when we port back SM token migration
     */
    void handleNewDlt(const DLT* dlt);

    /**
     * Translation from token or object ID to SM token ID
     */
    static fds_token_id smTokenId(fds_token_id tokId);
    static fds_token_id smTokenId(const ObjectID& objId,
                                  fds_uint32_t bitsPerToken);
    fds_token_id smTokenId(const ObjectID& objId);

    /**
     * Return a set of SM tokens that this SM currently owns
     */
    SmTokenSet getSmTokens() const;

    /**
     * Get disk ID where ObjectID (or SM token) data and metadata
     * resides on a given tier.
     */
    fds_uint16_t getDiskId(const ObjectID& objId,
                           diskio::DataTier tier);
    fds_uint16_t getDiskId(fds_token_id smTokId,
                           diskio::DataTier tier);

    /**
     * Get the root path to disk for a given SM token and tier
     */
    const char* getDiskPath(fds_token_id smTokId,
                            diskio::DataTier tier);

    /**
     * Module methods
     */
    virtual int mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  private:
    fds_uint32_t bitsPerToken_;

    //
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMDISKMAP_H_
