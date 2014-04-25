/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Volume metadata class. Describes the per-volume metada
 * that is maintained by a data manager.
 */

#ifndef SOURCE_DATA_MANAGER_VOLUMEMETA_H_
#define SOURCE_DATA_MANAGER_VOLUMEMETA_H_

#include <string>

#include <fds_types.h>
#include <fds_error.h>
#include <util/Log.h>

/* TODO: avoid cross compont include, move to include directory. */
#include <lib/VolumeCatalog.h>
#include <concurrency/Mutex.h>
#include <fds_volume.h>
#include <ObjectId.h>

namespace fds {

class MetadataPair {
  public:
    std::string key;
    std::string value;
    const char delim = ':';
    const std::string open_seq = "{";
    const std::string end_seq = "}";

    std::string ToString() const {
      return (open_seq + key + delim + value + end_seq);
    }

    void initFromString(std::string& str) {
      size_t pos = str.find(delim);
      key = str.substr(open_seq.size(), (pos - open_seq.size()));
      value = str.substr(pos + 1,
                         str.size() -1 - pos - end_seq.size());
      LOGDEBUG << "Built metadata pair " << *this
               << " from string " << str;
    }

    MetadataPair(const std::string &_key,
                 const std::string &_value) {
        key   = _key;
        value = _value;
    }

    MetadataPair(std::string& str) {
      initFromString(str);
    }

    MetadataPair(FDS_ProtocolInterface::FDSP_MetaDataPair mpair) {
      key = mpair.key;
      value = mpair.value;
    }
};
typedef std::vector<MetadataPair> MetaList;

std::ostream& operator<<(std::ostream& out, const MetadataPair& mdPair);

class BlobObjectInfo {
  public:
    fds_uint64_t offset;
    ObjectID data_obj_id;
    fds_uint64_t size;
    fds_bool_t sparse;
    fds_bool_t blob_end;
    const char delim = ':';
    const std::string open_seq = "{";
    const std::string end_seq = "}";

    BlobObjectInfo()
            : offset(0),
              size(0),
              sparse(false),
              blob_end(false) {
    }

    BlobObjectInfo(FDS_ProtocolInterface::FDSP_BlobObjectInfo& blob_obj_info)
            : BlobObjectInfo() {
        offset = blob_obj_info.offset;
        data_obj_id.SetId((const char *)blob_obj_info.data_obj_id.digest.c_str(), blob_obj_info.data_obj_id.digest.length());
        size = blob_obj_info.size;
        sparse = false;
        blob_end = blob_obj_info.blob_end;
    }

    BlobObjectInfo(const BlobObjectInfo &rhs) {
        *this = rhs;
    }

    /**
     * Constructor for a 'sparse' entry
     */
    BlobObjectInfo(fds_uint64_t off,
                   fds_uint64_t _size)
            : BlobObjectInfo() {
        offset = off;
        size   = _size;
        sparse = true;
    }

    BlobObjectInfo(std::string& str)
            : BlobObjectInfo() {
        initFromString(str);
    }

    ~BlobObjectInfo() {
    }

    /**
     * Serializes the blob object info into a string.
     * TODO(Andrew): This can just return a char * and
     * avoid the extra copy into a string.
     */
    std::string ToString() const {
        // TODO(Andrew): Add back a better serialization than ASCII.
        // We need ASCII for now because all of the parent classes
        // expect it, so they all need to change together.
        /*
        size_t serialSize = sizeof(offset) + ObjIdGen::getIdLength() + sizeof(size);
        size_t pos = 0;
        unsigned char serialBuf[serialSize];
        memcpy(serialBuf + pos, (const void *)&offset, sizeof(offset));
        pos += sizeof(offset);
        memcpy(serialBuf + pos, (const void *)data_obj_id.GetId(), ObjIdGen::getIdLength());
        pos += ObjIdGen::getIdLength();
        memcpy(serialBuf + pos, (const void *)&size, sizeof(size));

        std::string serialStr((const char *)serialBuf, serialSize);
        return serialStr;
        */

        std::ostringstream binfoOss;
        binfoOss << offset << delim
                 << "0x" << data_obj_id.ToString() << delim
                 << size << delim
                 << sparse << delim
                 << blob_end;
        LOGDEBUG << "Serialized blob object info " << *this
                 << " to " << binfoOss.str();

        return binfoOss.str();
    }

    /**
     * Deserializes the blob object info from a string.
     * TODO(Andrew): This can just take a char * and
     * avoid the extra copy into a string.
     */
    void initFromString(std::string& str) {
        // TODO(Andrew): Add back binary serialization
        /*
        size_t pos = 0;
        const char *serialBuf = str.c_str();
        offset = *((const fds_uint64_t *)serialBuf);
        pos += sizeof(offset);

        data_obj_id.SetId(serialBuf + pos, ObjIdGen::getIdLength());
        pos += ObjIdGen::getIdLength();

        size = *((const fds_uint64_t *)(serialBuf + pos));
        */
        int next_start = 0;
        int next_end = 0;
        ulong next_delim_pos = -1;
        std::string next_sub_str = "";
        for (fds_uint32_t field_idx = 0; field_idx < 5; field_idx++) {
            next_delim_pos = str.find(delim, next_start);
            if (next_delim_pos == std::string::npos) {
                fds_verify(field_idx == 4);
                next_end = str.size() - 1;
            } else {
                next_end = next_delim_pos- 1 ; 
            }
            next_sub_str = str.substr(next_start, next_end-next_start + 1);
            switch (field_idx) {
                case 0:
                    offset = strtoull(next_sub_str.c_str(), NULL, 0);
                    break;
                case 1:
                    data_obj_id = next_sub_str;
                    break;
                case 2:
                    size = strtoull(next_sub_str.c_str(), NULL, 0);
                    break;
                case 3:
                    sparse = (fds_bool_t)strtoul(next_sub_str.c_str(), NULL, 0);
                    break;
                case 4:
                    blob_end = (fds_bool_t)strtoul(next_sub_str.c_str(), NULL, 0);
                    break;
                default:
                    fds_panic("Unknown blob object info field!");
            }
            next_start = next_end+2;
        }

        LOGDEBUG << "From string " << str << " created blob object info "
                 << *this;
    }

    BlobObjectInfo& operator=(const BlobObjectInfo &rhs) {
        offset      = rhs.offset;
        data_obj_id = rhs.data_obj_id;
        size        = rhs.size;
        sparse      = rhs.sparse;
        blob_end    = rhs.blob_end;
        return *this;
    }
};

std::ostream& operator<<(std::ostream& out, const BlobObjectInfo& binfo);

class BlobObjectList {

  private:
    std::vector<BlobObjectInfo> obj_list;

  public:
    const char delim = ',';
    const std::string open_seq = "[";
    const std::string end_seq = "]";

    BlobObjectList() {
        obj_list.clear();
    }

    BlobObjectList(std::string& str) :
            BlobObjectList() {
        initFromString(str);
    }

    BlobObjectList(FDS_ProtocolInterface::FDSP_BlobObjectList& blob_obj_list)
            : BlobObjectList() {
        initFromFDSPObjList(blob_obj_list);
    }

    ~BlobObjectList() {
    }

    const BlobObjectInfo& operator[](const int index) const {
        return obj_list[index];
    }

    BlobObjectInfo& operator[](const int index) {
        return obj_list[index];
    }

    void pushBack(const BlobObjectInfo &bInfo) {
        obj_list.push_back(bInfo);
    }

    void popBack() {
        obj_list.pop_back();
    }

    BlobObjectList& operator=(const BlobObjectList &rhs) {
        obj_list = rhs.obj_list;
        return *this;
    }

    const BlobObjectInfo& back() const {
        return obj_list.back();
    }

    typedef std::vector<BlobObjectInfo>::iterator iterator;
    typedef std::vector<BlobObjectInfo>::const_iterator const_iterator;
    const_iterator cbegin() const {
        return obj_list.cbegin();
    }

    const_iterator cend() const {
        return obj_list.cend();
    }

    const BlobObjectInfo& objectForOffset(const fds_uint64_t offset) const {
        uint i;
        for (i = 0; 
             ((i < obj_list.size()) && (obj_list[i].offset < offset)); 
             i++)
            ;
        if (i == obj_list.size()) {
            throw "Offset out of bound";
        }
        if (obj_list[i].offset == offset)
            return obj_list[i];
        return obj_list[i-1];
    }

    fds_uint32_t size() const {
        return obj_list.size();
    }

    std::string ToString() const {
        uint i;
        
        if (obj_list.size() == 0) {
            return (open_seq + end_seq);
        }
      
        std::string ret_str = open_seq;
        for (i = 0; i < obj_list.size()-1; i++) {
            ret_str += obj_list[i].ToString() + delim;
        }
        ret_str += obj_list[i].ToString();
        ret_str += end_seq;
        return ret_str;
    }

    void initFromString(std::string& str) {
        obj_list.clear();

        ulong next_delim_pos = std::string::npos;
        int next_start = open_seq.size();
        int next_end = 0;
        std::string next_sub_str = "";
        bool last_obj = false;

        if (str == open_seq + end_seq) {
            return;
        }

        while (!last_obj) {
            next_delim_pos = str.find(delim, next_start);
            if (next_delim_pos == std::string::npos) {
                next_end = str.size()- 1- end_seq.size();
                last_obj = true;
            } else {
                next_end = next_delim_pos-1;
            }
            next_sub_str = str.substr(next_start, next_end-next_start+1);
            if (next_sub_str != "") {
                BlobObjectInfo obj_info(next_sub_str);
                obj_list.push_back(obj_info);
            }
            next_start = next_end+2;
        }
    }

    void initFromFDSPObjList(FDS_ProtocolInterface::FDSP_BlobObjectList& blob_obj_list) {
        obj_list.clear();
        for (uint i = 0; i < blob_obj_list.size(); i++) {
            obj_list.push_back(blob_obj_list[i]);
        }
    }

    void ToFDSPObjList(FDS_ProtocolInterface::FDSP_BlobObjectList& fdsp_obj_list) const {
        fdsp_obj_list.clear();
        uint i = 0;
        for (i = 0; i < obj_list.size(); i++) {
            FDS_ProtocolInterface::FDSP_BlobObjectInfo obj_info;
            obj_info.offset = obj_list[i].offset;
            obj_info.size = obj_list[i].size;
            obj_info.data_obj_id.digest = std::string((const char *)(obj_list[i].data_obj_id.GetId()), (size_t)obj_list[i].data_obj_id.GetLen());
            //	obj_info.data_obj_id.hash_low = obj_list[i].data_obj_id.GetLow();
            fdsp_obj_list.push_back(obj_info);
        }
    }
  };

/**
 * Metadata that describes the blob. Analagous to
 * a file inode in a file system.
 */
class BlobNode {
  public:
    std::string blob_name;
    blob_version_t version;
    fds_volid_t vol_id;
    fds_uint64_t blob_size;
    fds_uint32_t blob_mime_type;
    fds_uint32_t replicaCnt;
    fds_uint32_t writeQuorum;
    fds_uint32_t readQuorum;
    fds_uint32_t consisProtocol;
    MetaList meta_list; /// List of metadata key-value pairs
    BlobObjectList obj_list;
      
    const char delim = ';';
    const std::string open_seq = "";
    const std::string end_seq = "";

    BlobNode() {
        // Init the new blob to an invalid version
        // until someone actually inits valid data.
        version = blob_version_invalid;
        blob_size = 0;
        meta_list.clear();
    }

    ~BlobNode() {
    }

    BlobNode(fds_volid_t volId, const std::string &name)
            : BlobNode() {
        vol_id = volId;
        blob_name = name;
    }

    BlobNode(std::string& str)
            : BlobNode() {
        initFromString(str);
    }

    BlobNode(FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr cat_msg, fds_volid_t _vol_id)
            : BlobNode() {
        initFromFDSPPayload(cat_msg, _vol_id);
    }

    void updateMetadata(const std::string &key,
                        const std::string &value) {
        for (MetaList::iterator it = meta_list.begin();
             it != meta_list.end();
             it++) {
            if ((*it).key == key) {
                // We already store the key, overwrite
                (*it).value = value;
                LOGNORMAL << "Modified metadata for blob " << blob_name
                          << " key=" << key << " value=" << value;
                return;
            }
        }

        // We don't already have the key, append it
        meta_list.push_back(MetadataPair(key, value));
        LOGNORMAL << "Added metadata for blob " << blob_name
                  << " key=" << key << " value=" << value;
    }

    std::string metaListToString() const {
        uint i;
        if (meta_list.size() == 0) {
            return "[]";
        }
        std::string ret_str = "[";
        for (i = 0; i < meta_list.size()-1; i++) {
            ret_str += meta_list[i].ToString() + ",";
        }
        ret_str += meta_list[i].ToString();
        ret_str += "]";
        return ret_str;
    }

    void initMetaListFromString(std::string& str) {
        meta_list.clear();

        int last_pos = 1;
        ulong next_pos = 0;
        std::string next_sub_str = "";
        bool last_pair = false;

        if (str == "[]") {
            return;
        }

        while (!last_pair) {
            next_pos = str.find(',', last_pos + 1);
            if (next_pos == std::string::npos) {
                next_pos = str.size()- 1;
                last_pair = true;
            }
            next_sub_str = str.substr(last_pos, next_pos - last_pos);
            if (next_sub_str != "") {
                MetadataPair mpair(next_sub_str);
                meta_list.push_back(mpair);
            }
            last_pos = next_pos;
        }
    }

    void initMetaListFromFDSPMetaList(FDS_ProtocolInterface::FDSP_MetaDataList& mlist) {
        meta_list.clear();
        uint i = 0;
        for (i = 0; i < mlist.size(); i++) {
            meta_list.push_back(mlist[i]);
        }
    }

    void metaListToFDSPMetaList(FDS_ProtocolInterface::FDSP_MetaDataList& mlist) const {
        mlist.clear();
        uint i = 0;
        for (i = 0; i < meta_list.size(); i++) {
            FDS_ProtocolInterface::FDSP_MetaDataPair mpair;
            mpair.key = meta_list[i].key;
            mpair.value = meta_list[i].value;
            mlist.push_back(mpair);
        }
    }

    std::string ToString() const { 
        std::ostringstream bnode_oss;
        bnode_oss << blob_name << delim << vol_id << delim
                  << version << delim << blob_size << delim
                  << metaListToString() << delim << obj_list.ToString();
        return bnode_oss.str();
    }

    void initFromString(std::string& str) {
        // Since we're updaing the blob's contents,
        // bumps its version.
        // TODO(Andrew): This should be based on the vols versioning
        version++;
        blob_mime_type = 0;
        replicaCnt = writeQuorum = readQuorum = 0;
        consisProtocol = 0;

        int next_start = 0;
        int next_end = 0;
        ulong next_delim_pos = -1;
        std::string next_sub_str = "";
        int field_idx = 0;

        while (field_idx < 6) {
            next_delim_pos = str.find(delim, next_start);
            if (next_delim_pos == std::string::npos) {
                fds_verify(field_idx == 5);
                next_end = str.size()-1;
            } else {
                next_end = next_delim_pos-1; 
            }
            next_sub_str = str.substr(next_start, next_end-next_start+1);
            switch (field_idx) {
                case 0:
                    blob_name = next_sub_str;
                    break;
                case 1:
                    vol_id = strtoull(next_sub_str.c_str(), NULL, 0);
                    break;
                case 2:
                    version = strtoull(next_sub_str.c_str(), NULL, 0);
                    break;
                case 3:
                    blob_size = strtoull(next_sub_str.c_str(), NULL, 0);
                    break;
                case 4:
                    initMetaListFromString(next_sub_str);
                    break;
                case 5:
                    obj_list.initFromString(next_sub_str);
                    break;
                default:
                    fds_verify(0);
            }
            next_start = next_end+2;
            field_idx++;
        }
    }

    void initFromFDSPPayload(const FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr cat_msg,
                             fds_volid_t _vol_id) {
        // Since we're updaing the blob's contents,
        // bumps its version.
        // TODO(Andrew): This should be based on the vols versioning
        version++;
        blob_mime_type = 0;
        replicaCnt = writeQuorum = readQuorum = 0;
        consisProtocol = 0;

        vol_id = _vol_id;

        blob_name = cat_msg->blob_name;
 
        initMetaListFromFDSPMetaList(cat_msg->meta_list);
        obj_list.initFromFDSPObjList(cat_msg->obj_list);
      
        // TODO(Andrew): This calculation assumes that we're
        // setting the entire blobs contents. This needs to
        // change when we allow partial updates or sparse blobs.
        blob_size = 0;
        for (fds_uint32_t i = 0; i < obj_list.size(); i++) {
            blob_size += (obj_list[i]).size;
        }
    }

    void ToFdspPayload(FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr& query_msg) const {
        query_msg->blob_name = blob_name;
        query_msg->blob_size = blob_size;
        query_msg->blob_version = version;
        metaListToFDSPMetaList(query_msg->meta_list);
        obj_list.ToFDSPObjList(query_msg->obj_list);
    }
};

std::ostream& operator<<(std::ostream& out, const BlobNode& bnode);

class VolumeMeta {
  private:
    fds_mutex  *vol_mtx;

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

    /*
     * A logger received during instantiation.
     * It is allocated and destroyed by the
     * caller.
     */
    fds_log *dm_log;

    /*
     * This class is non-copyable.
     * Disable copy constructor/assignment operator.
     */
    VolumeMeta(const VolumeMeta& _vm);
    VolumeMeta& operator=(const VolumeMeta rhs);

 public:

    VolumeCatalog *getVcat();
    VolumeDesc *vol_desc;
    /*
     * Default constructor should NOT be called
     * directly. It is needed for STL data struct
     * support.
     */
    VolumeMeta();
    VolumeMeta(const std::string& _name,
               fds_volid_t _uuid,VolumeDesc *v_desc);
    VolumeMeta(const std::string& _name,
               fds_volid_t _uuid,
               fds_log* _dm_log,VolumeDesc *v_desc);
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
};

}  // namespace fds

#endif  // SOURCE_DATA_MANAGER_VOLUMEMETA_H_
