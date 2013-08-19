#include <Ice/Ice.h>
#include <FDSP.h>
#include <ObjectFactory.h>
#include <Ice/ObjectFactory.h>
#include <Ice/BasicStream.h>
#include <Ice/Object.h>
#include <IceUtil/Iterator.h>
#include "StorHvisorNet.h"
//#include "hvisor_lib.h"
#include "StorHvisorCPP.h"

StorHvCtrl *storHvisor;
using namespace std;
using namespace FDS_ProtocolInterface;
using namespace Ice;


struct fbd_device *fbd_dev;
extern vvc_vhdl_t vvc_vol_create(volid_t vol_id, const char *db_name, int max_blocks);

void CreateStorHvisor(int argc, char *argv[])
{
     storHvisor = new StorHvCtrl(argc, argv);
}


StorHvCtrl::StorHvCtrl(int argc, char *argv[])
{
        Ice::InitializationData initData;
        initData.properties = Ice::createProperties();
	initData.properties->load("fds.conf");
        _communicator = Ice::initialize(argc, argv, initData);
	Ice::PropertiesPtr props = _communicator->getProperties();

        std::string storMgrIPAddress  = props->getProperty("ObjectStorMgrSvr.IPAddress");
        int storMgrPortNum  = props->getPropertyAsInt("ObjectStorMgrSvr.PortNumber");
        std::string dataMgrIPAddress  = props->getProperty("DataMgr.IPAddress");
        int dataMgrPortNum  = props->getPropertyAsInt("DataMgr.PortNumber");

        props->setProperty("StorHvisorClient.ThreadPool.Size", "10");
        props->setProperty("StorHvisorClient.ThreadPool.SizeMax", "20");
        props->setProperty("StorHvisorClient.ThreadPool.SizeWarn", "18");
      
       rpcSwitchTbl = new FDS_RPC_EndPointTbl(_communicator); 
 

        rpcSwitchTbl->Add_RPC_EndPoint(storMgrIPAddress, storMgrPortNum, FDSP_STOR_MGR);
        rpcSwitchTbl->Add_RPC_EndPoint(dataMgrIPAddress, dataMgrPortNum, FDSP_DATA_MGR);
}

StorHvCtrl::~StorHvCtrl()
{
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
#if 0
        if((fbd_dev->vhdl = vvc_vol_create(fbd_dev->vol_id, NULL,8192)) == 0 )
        {
                printf(" Error: creating the  vvc  volume \n");
                goto out;
        }


        rc = pthread_create(&fbd_dev->rx_thread, NULL, fds_rx_io_proc, fbd_dev);
        if (rc) {
          printf(" Error: creating the   RX IO thread : %d \n", rc);
                goto out;
        }
#endif

        return (fbd_dev);
        return (void *)err;
}
END_C_DECLS

