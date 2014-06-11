/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Volume metadata class. Describes the per-volume metada
 * that is maintained by a data manager.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_VOLUMEMETA_H_
#define SOURCE_DATA_MGR_INCLUDE_VOLUMEMETA_H_

#include <string>
#include <list>
#include <vector>
#include <map>
#include <fds_types.h>
#include <fds_error.h>
#include <dm-platform.h>
#include <util/Log.h>

#include <lib/VolumeCatalog.h>
#include <concurrency/Mutex.h>
#include <fds_volume.h>
#include <ObjectId.h>
#include <fdsp/fds_service_types.h>
#include <serialize.h>

namespace fds {

struct MetadataPair : serialize::Serializable {
    std::string key;
    std::string value;
    MetadataPair();
    MetadataPair(const std::string &_key, const std::string &_value);
    MetadataPair(fpi::FDSP_MetaDataPair mpair);  // NOLINT

    uint32_t write(serialize::Serializer*  s) const;
    uint32_t read(serialize::Deserializer* d);
};

typedef std::vector<MetadataPair> MetaList;

std::ostream& operator<<(std::ostream& out, const MetadataPair& mdPair);
std::ostream& operator<<(std::ostream& out, const MetaList& metaList);

struct BlobObjectInfo : serialize::Serializable {
    fds_uint64_t offset = 0;
    ObjectID data_obj_id;
    fds_uint64_t size = 0;
    fds_bool_t sparse = false;
    fds_bool_t blob_end = false;

    BlobObjectInfo();
    BlobObjectInfo(const fpi::FDSP_BlobObjectInfo& blob_obj_info); // NOLINT

    BlobObjectInfo(const BlobObjectInfo &rhs);
    BlobObjectInfo(fds_uint64_t off, fds_uint64_t _size);
    ~BlobObjectInfo();

    uint32_t write(serialize::Serializer*  s) const;
    uint32_t read(serialize::Deserializer* d);

    BlobObjectInfo& operator= (const BlobObjectInfo &rhs);
};

std::ostream& operator<<(std::ostream& out, const BlobObjectInfo& binfo);

struct BlobObjectList : std::map<fds_uint64_t, BlobObjectInfo>, serialize::Serializable {
    BlobObjectList();
    BlobObjectList(fpi::FDSP_BlobObjectList& blob_obj_list);  // NOLINT
    ~BlobObjectList();

    fds_uint32_t write(serialize::Serializer*  s) const;
    fds_uint32_t read(serialize::Deserializer* d);

    bool hasObjectAtOffset(fds_uint64_t offset) const;
    const BlobObjectInfo& objectAtOffset(const fds_uint64_t offset) const;
    void initFromFDSPObjList(fpi::FDSP_BlobObjectList& blob_obj_list);
    void ToFDSPObjList(fpi::FDSP_BlobObjectList& fdsp_obj_list) const;
};

std::ostream& operator<<(std::ostream& out, const BlobObjectList& blobObjectList);

/**
 * Metadata that describes the blob. Analagous to
 * a file inode in a file system.
 */
struct BlobNode : serialize::Serializable {
    std::string blob_name;
    blob_version_t version = 0;
    fds_volid_t vol_id = 0;
    fds_uint64_t blob_size = 0;
    fds_uint32_t blob_mime_type = 0;
    fds_uint32_t replicaCnt = 0;
    fds_uint32_t writeQuorum = 0;
    fds_uint32_t readQuorum = 0;
    fds_uint32_t consisProtocol = 0;
    MetaList meta_list;
    BlobObjectList obj_list;

    typedef boost::shared_ptr<BlobNode> ptr;
    typedef boost::shared_ptr<const BlobNode> const_ptr;

    BlobNode();
    virtual ~BlobNode();

    BlobNode(fds_volid_t volId, const std::string &name);
    BlobNode(fpi::FDSP_UpdateCatalogTypePtr cat_msg, fds_volid_t _vol_id);

    void updateMetadata(const std::string &key, const std::string &value);

    uint32_t write(serialize::Serializer*  s) const;
    uint32_t read(serialize::Deserializer* d);

    void initMetaListFromFDSPMetaList(fpi::FDSP_MetaDataList& mlist);
    void metaListToFDSPMetaList(fpi::FDSP_MetaDataList& mlist) const;
    void initFromFDSPPayload(const fpi::FDSP_UpdateCatalogTypePtr cat_msg, fds_volid_t _vol_id);
    void ToFdspPayload(fpi::FDSP_QueryCatalogTypePtr& query_msg) const;
    void ToFdspPayload(fpi::QueryCatalogMsgPtr& query_msg) const;
};

std::ostream& operator<<(std::ostream& out, const BlobNode& bnode);

class VolumeMeta : public HasLogger {
  private:
    fds_mutex  *vol_mtx;

    /*
     * This class is non-copyable.
     * Disable copy constructor/assignment operator.
     */
    VolumeMeta(const VolumeMeta& _vm);
    VolumeMeta& operator= (const VolumeMeta rhs);

 public:
    /*
     * The volume catalog maintains mappings from
     * vol/blob/offset to object id.
     */
    VolumeCatalog *vcat;
    /*
     * The time catalog maintains pending changes to
     * the volume catalog.
     */
    TimeCatalog *tcat;

    VolumeCatalog *getVcat();
    VolumeDesc *vol_desc;
    /*
     * Default constructor should NOT be called
     * directly. It is needed for STL data struct
     * support.
     */
    VolumeMeta();
    VolumeMeta(const std::string& _name,
               fds_volid_t _uuid,
               VolumeDesc *v_desc,
               fds_bool_t crt_catalogs);
    VolumeMeta(const std::string& _name,
               fds_volid_t _uuid,
               fds_log* _dm_log,
               VolumeDesc *v_desc,
               fds_bool_t crt_catalogs);
    ~VolumeMeta();
    void dmCopyVolumeDesc(VolumeDesc *v_desc, VolumeDesc *pVol);
    /*
     * per volume queue
     */
    FDS_VolumeQueue*  dmVolQueue;


    Error OpenTransaction(const std::string blob_name,
                          const BlobNode*& bnode, VolumeDesc *pVol);
    Error QueryVcat(const std::string blob_name,
                    BlobNode*& bnode);
    Error DeleteVcat(const std::string blob_name);
    fds_bool_t isEmpty() const;
    Error listBlobs(std::list<BlobNode>& bNodeList);
    Error syncVolCat(fds_volid_t volId, NodeUuid node_uuid);
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_VOLUMEMETA_H_
