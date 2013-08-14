#include <Ice/Ice.h>
#include <FDSP.h>
#include <ObjectFactory.h>
#include <Ice/ObjectFactory.h>
#include <Ice/BasicStream.h>
#include <Ice/Object.h>
#include <IceUtil/Iterator.h>
#include <StorHvisorClient.h>


using namespace std;
using namespace FDS_ProtocolInterface;

class FDSP_DataPathRespCbackI : public FDSP_DataPathResp
{
public:
    void GetObjectResp(const FDSP_MsgHdrTypePtr&, const FDSP_GetObjTypePtr&, const Ice::Current&) {
    }

    void PutObjectResp(const FDSP_MsgHdrTypePtr&, const FDSP_PutObjTypePtr&, const Ice::Current&) {
    }

    void UpdateCatalogObjectResp(const FDSP_MsgHdrTypePtr& fdsp_msg, const FDSP_UpdateCatalogTypePtr& cat_obj_req, const Ice::Current &) {
    }

    void OffsetWriteObjectResp(const FDSP_MsgHdrTypePtr& fdsp_msg, const FDSP_OffsetWriteObjTypePtr& offset_write_obj_req, const Ice::Current &) {

    }
    void RedirReadObjectResp(const FDSP_MsgHdrTypePtr& fdsp_msg, const FDSP_RedirReadObjTypePtr& redir_write_obj_req, const Ice::Current &)
    { 
    }
};


StorHvisorClient::StorHvisorClient()
{
}

int
StorHvisorClient::run(int argc, char* argv[])
{
    int status = 0;

	 FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr = new FDSP_MsgHdrType;
	 // FDS_ProtocolInterface::FDSP_PutObjTypePtr put_obj_req = new FDSP_PutObjType;
         FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr up_cat_req = new FDSP_UpdateCatalogType;

    try {
         FDSP_DataPathReqPrx fdspDPAPI = FDSP_DataPathReqPrx::checkedCast(communicator()->propertyToProxy ("StorHvisorClient.Proxy"));
        if (!fdspDPAPI)
            throw "Invalid proxy";

        Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("");
        if (!adapter)
            throw "Invalid adapter";

        Ice::Identity ident;
        ident.name = IceUtil::generateUUID();
        ident.category = "";

        FDSP_DataPathRespPtr fdspDataPathRespCback = new FDSP_DataPathRespCbackI;
        if (!fdspDataPathRespCback)
            throw "Invalid fdspDataPathRespCback";

        adapter->add(fdspDataPathRespCback, ident);
        adapter->activate();
        fdspDPAPI->ice_getConnection()->setAdapter(adapter);
        //fdspDPAPI->addClient(ident);


	fdsp_msg_hdr->minor_ver = 0;
	// fdsp_msg_hdr->msg_code = FDSP_MSG_PUT_OBJ_REQ;
        fdsp_msg_hdr->msg_code = FDSP_MSG_UPDATE_CAT_OBJ_REQ;
        fdsp_msg_hdr->msg_id =  1;

        fdsp_msg_hdr->major_ver = 0xa5;
        fdsp_msg_hdr->minor_ver = 0x5a;

        fdsp_msg_hdr->num_objects = 1;
        fdsp_msg_hdr->frag_len = 0;
        fdsp_msg_hdr->frag_num = 0;

        fdsp_msg_hdr->tennant_id = 0;
        fdsp_msg_hdr->local_domain_id = 0;
        fdsp_msg_hdr->glob_volume_id = 0;

        fdsp_msg_hdr->src_id = FDSP_STOR_HVISOR;
        // fdsp_msg_hdr->dst_id = FDSP_STOR_MGR;
        fdsp_msg_hdr->dst_id = FDSP_DATA_MGR;

        fdsp_msg_hdr->err_code=FDSP_ERR_SM_NO_SPACE;
        fdsp_msg_hdr->result=FDSP_ERR_OK;

        up_cat_req->volume_offset = 1234;
        up_cat_req->dm_transaction_id = 6789;
        up_cat_req->data_obj_id.hash_high = 0xdead;
        up_cat_req->data_obj_id.hash_low = 0xbeef;
        up_cat_req->dm_operation = 8888;

        // fdspDPAPI->PutObject(fdsp_msg_hdr, put_obj_req);
        fdspDPAPI->UpdateCatalogObject(fdsp_msg_hdr, up_cat_req);
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
        status = 1;
    } catch (const char* msg) {
        cerr << msg << endl;
        status = 1;
    }
    return status;
}

int
main(int argc, char* argv[])
{
    StorHvisorClient storHvisorClient;
    return storHvisorClient.main(argc, argv, "fds.conf");
}
