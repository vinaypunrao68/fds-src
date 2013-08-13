#include <Ice/Ice.h>
#include <FDSP.h>
#include <ObjectFactory.h>
#include <Ice/ObjectFactory.h>
#include <Ice/BasicStream.h>
#include <Ice/Object.h>
#include <IceUtil/Iterator.h>
#include "ubd.h"
#include "StorHvisorNet.h"
//#include "hvisor_lib.h"
#include "StorHvisorCPP.h"


using namespace std;
using namespace FDS_ProtocolInterface;


FDSP_NetworkCon  NETPtr;
struct fbd_device *fbd_dev;
extern vvc_vhdl_t vvc_vol_create(volid_t vol_id, const char *db_name, int max_blocks);

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

BEGIN_C_DECLS
void init_DPAPI()
{
	FDSP_NetworkRec   netRec;

    try {
		NETPtr.InitIceObejcts();
		NETPtr.CreateNetworkEndPoint(&netRec);

    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
    }
}
END_C_DECLS

void FDSP_NetworkCon::InitIceObejcts()
{
	NETPtr.initData.properties->load("fds.conf");
}

void FDSP_NetworkCon::CreateNetworkEndPoint( FDSP_NetworkRec *netRec )
{
	Ice::CommunicatorPtr icDM = Ice::initialize(NETPtr.initData);
	NETPtr.fdspDPAPI = FDSP_DataPathReqPrx::checkedCast(icDM->propertyToProxy ("StorHvisorClient.Proxy"));

        Ice::ObjectAdapterPtr adapterDM = icDM->createObjectAdapter("");

        if (!adapterDM)
            throw "Invalid adapter";

        Ice::Identity ident;
        ident.name = IceUtil::generateUUID();
        ident.category = "";

        fdspDataPathResp  = new FDSP_DataPathRespCbackI;
        if (!fdspDataPathResp)
            throw "Invalid fdspDataPathRespCback";
        adapterDM->add(fdspDataPathResp, ident);
        adapterDM->add(fdspDataPathResp, ident);

        adapterDM->activate();

        NETPtr.fdspDPAPI->ice_getConnection()->setAdapter(adapterDM);
        NETPtr.fdspDPAPI->AssociateRespCallback(ident);
}



BEGIN_C_DECLS
void *hvisor_lib_init(void)
{
        int err = -ENOMEM;
//        int rc;

        fbd_dev = (fbd_device *)malloc(sizeof(*fbd_dev));
        if (!fbd_dev)
          return (void *)-ENOMEM;
        memset(fbd_dev, 0, sizeof(*fbd_dev));

        fbd_dev->blocksize = 4096; /* default value  */
        fbd_dev->bytesize = 0;
        /* we will have to  generate the dev id for multiple device support */
        fbd_dev->dev_id = 0;

        fbd_dev->vol_id = 1; /*  this should be comming from OM */
        if((fbd_dev->vhdl = vvc_vol_create(fbd_dev->vol_id, NULL,8192)) == 0 )
        {
                printf(" Error: creating the  vvc  volume \n");
                goto out;
        }

#if 0
        rc = pthread_create(&fbd_dev->rx_thread, NULL, fds_rx_io_proc, fbd_dev);
        if (rc) {
          printf(" Error: creating the   RX IO thread : %d \n", rc);
                goto out;
        }
#endif

        return (fbd_dev);
out:
        free(fbd_dev);
        return (void *)err;
}
END_C_DECLS

