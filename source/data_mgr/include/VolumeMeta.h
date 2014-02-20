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
#include <fds_err.h>
#include <util/Log.h>

/* TODO: avoid cross compont include, move to include directory. */
#include <lib/VolumeCatalog.h>
#include <concurrency/Mutex.h>
#include <fds_volume.h>

namespace fds {

  typedef int blob_version_t;

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
      key = str.substr(open_seq.size(), pos);
      value = str.substr(pos+1, str.size()-1-pos-end_seq.size());
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

  class BlobObjectInfo {
  public:
    fds_uint64_t offset;
    ObjectID data_obj_id;
    fds_uint64_t size;
    const char delim = ':';
    const std::string open_seq = "{";
    const std::string end_seq = "}";

    BlobObjectInfo(FDS_ProtocolInterface::FDSP_BlobObjectInfo& blob_obj_info) {
      offset = blob_obj_info.offset;
      data_obj_id.SetId(blob_obj_info.data_obj_id.hash_high, blob_obj_info.data_obj_id.hash_low);
      size = blob_obj_info.size;
    }

    std::string ToString() const {
#if 0
      return (open_seq 
	      + std::to_string(offset) + delim
	      + std::to_string(data_obj_id.GetHigh()) + delim
	      + std::to_string(data_obj_id.GetLow()) + delim
	      + std::to_string(size)
	      + end_seq);
#endif
      char *out_str = (char *)malloc(sizeof(char) * 512);
      sprintf(out_str, "{%llu:%llx:%llx:%llu}", 
	      offset, data_obj_id.GetHigh(), data_obj_id.GetLow(), size);
      std::string ret_str(out_str);
      free(out_str);
      return ret_str;
    }

    void initFromString(std::string& str) {
#if 0
      size_t pos1 = str.find(delim);
      offset = strtoull(str.substr(open_seq.size(), pos1).c_str(), NULL, 0);
      size_t pos2 = str.find(delim, pos1+1);
      fds_uint64_t hash_hi = strtoull(str.substr((pos1+1,pos2-pos1-1)).c_str(), NULL, 0);
      size_t pos3 = str.find(delim, pos2+1);
      fds_uint64_t hash_lo = strtoull(str.substr((pos2+1,pos3-pos2-1)).c_str(), NULL, 0);
      data_obj_id.SetId(hash_hi, hash_lo);
      size = strtoull(str.substr(pos3+1, str.size()-1-pos3-end_seq.size()).c_str(), NULL, 0);
#endif
      fds_uint64_t hash_hi, hash_lo;
      sscanf(str.c_str(), "{%llu:%llx:%llx:%llu}",
	     &offset, &hash_hi, &hash_lo, &size);
      data_obj_id.SetId(hash_hi, hash_lo);
      
    }
 
    BlobObjectInfo(std::string& str) {
      initFromString(str);
    }
  };

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

    const BlobObjectInfo& operator[](const int index) const {
      return obj_list[index];
    }

    const BlobObjectInfo& objectForOffset(const fds_uint64_t offset) const {
      int i;
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
      int i;

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

      int next_delim_pos = -1;
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

    BlobObjectList(std::string& str) {
      initFromString(str);
    }

    void initFromFDSPObjList(FDS_ProtocolInterface::FDSP_BlobObjectList& blob_obj_list) {
      obj_list.clear();
      for (int i = 0; i < blob_obj_list.size(); i++) {
	obj_list.push_back(blob_obj_list[i]);
      }
    }
    
    BlobObjectList(FDS_ProtocolInterface::FDSP_BlobObjectList& blob_obj_list) {
      initFromFDSPObjList(blob_obj_list);
    }

    void ToFDSPObjList(FDS_ProtocolInterface::FDSP_BlobObjectList& fdsp_obj_list) const {

      fdsp_obj_list.clear();
      int i = 0;
      for (i = 0; i < obj_list.size(); i++) {
	FDS_ProtocolInterface::FDSP_BlobObjectInfo obj_info;
	obj_info.offset = obj_list[i].offset;
	obj_info.size = obj_list[i].size;
	obj_info.data_obj_id.hash_high = obj_list[i].data_obj_id.GetHigh();
	obj_info.data_obj_id.hash_low = obj_list[i].data_obj_id.GetLow();
	fdsp_obj_list.push_back(obj_info);
      }
    }
      

  };

  class BlobNode {
  public:
    std::string blob_name;
    blob_version_t current_version;
    fds_volid_t vol_id;
    fds_uint64_t blob_size;
    fds_uint32_t blob_mime_type;
    fds_uint32_t replicaCnt;
    fds_uint32_t writeQuorum;
    fds_uint32_t readQuorum;
    fds_uint32_t consisProtocol;
    MetaList meta_list;
    BlobObjectList obj_list;
    const char delim = ';';
    const std::string open_seq = "";
    const std::string end_seq = "";


    BlobNode() {
      meta_list.clear();
    }

    std::string metaListToString() const {
      int i;
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
      int next_pos = 0;
      std::string next_sub_str = "";
      bool last_pair = false;

      if (str == "[]") {
	return;
      }

      while (!last_pair) {
	next_pos = str.find(',', last_pos+1);
	if (next_pos == std::string::npos) {
	  next_pos = str.size()- 1;
	  last_pair = true;
	}
	next_sub_str = str.substr(last_pos+1, next_pos-last_pos-1);
	if (next_sub_str != "") {
	  MetadataPair mpair(next_sub_str);
	  meta_list.push_back(mpair);
	}
	last_pos = next_pos;
      }
    }

    void initMetaListFromFDSPMetaList(FDS_ProtocolInterface::FDSP_MetaDataList& mlist) {

      meta_list.clear();
      int i = 0;
      for (i = 0; i < mlist.size(); i++) {
	meta_list.push_back(mlist[i]);
      }
    }

    void metaListToFDSPMetaList(FDS_ProtocolInterface::FDSP_MetaDataList& mlist) const {
      mlist.clear();
      int i = 0;
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
		<< current_version << delim << blob_size << delim
		<< metaListToString() << delim << obj_list.ToString();
      return bnode_oss.str();
    }

    void initFromString(std::string& str) {

      current_version = 0;
      blob_mime_type = 0;
      replicaCnt = writeQuorum = readQuorum = 0;
      consisProtocol = 0;

      int next_start = 0;
      int next_end = 0;
      int next_delim_pos = -1;
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
	  current_version = strtoul(next_sub_str.c_str(), NULL, 0);
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

    BlobNode(std::string& str) {
      initFromString(str);
    }

    void initFromFDSPPayload(FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr& cat_msg, fds_volid_t _vol_id) {

      current_version = 0;
      blob_mime_type = 0;
      replicaCnt = writeQuorum = readQuorum = 0;
      consisProtocol = 0;

      vol_id = _vol_id;

      blob_name = cat_msg->blob_name;
      blob_size = cat_msg->blob_size;
 
      initMetaListFromFDSPMetaList(cat_msg->meta_list);
      obj_list.initFromFDSPObjList(cat_msg->obj_list);

    }

    BlobNode(FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr cat_msg, fds_volid_t _vol_id) {
      initFromFDSPPayload(cat_msg, _vol_id);
    }

    void ToFdspPayload(FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr& query_msg) const {
      query_msg->blob_name = blob_name;
      query_msg->blob_size = blob_size;
      metaListToFDSPMetaList(query_msg->meta_list);
      obj_list.ToFDSPObjList(query_msg->obj_list);
    }

  };


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
    Error listBlobs(std::list<BlobNode>& bNodeList);
  };

}  // namespace fds

#endif  // SOURCE_DATA_MANAGER_VOLUMEMETA_H_
