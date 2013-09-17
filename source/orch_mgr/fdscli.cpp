/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include "fdsCli.h"

using namespace FDSP_Types;
using namespace std;
using namespace FDS_ProtocolInterface;

namespace fds {
FdsCli  *fdsCli;


void FdsCli::InitCfgMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr)
{
        msg_hdr->minor_ver = 0;
        msg_hdr->msg_code = FDSP_MSG_PUT_OBJ_REQ;
        msg_hdr->msg_id =  1;

        msg_hdr->major_ver = 0xa5;
        msg_hdr->minor_ver = 0x5a;

        msg_hdr->num_objects = 1;
        msg_hdr->frag_len = 0;
        msg_hdr->frag_num = 0;

        msg_hdr->tennant_id = 0;
        msg_hdr->local_domain_id = 0;

        msg_hdr->src_id = FDSP_CLI_MGR;
        msg_hdr->dst_id = FDSP_ORCH_MGR;

        msg_hdr->err_code=FDSP_ERR_SM_NO_SPACE;
        msg_hdr->result=FDSP_ERR_OK;

}


int FdsCli::fdsCliPraser(int argc, char* argv[])
{
	FDSP_MsgHdrTypePtr msg_hdr = new FDSP_MsgHdrType;
	InitCfgMsgHdr(msg_hdr);

	namespace po = boost::program_options;
	// Declare the supported options.
        po::options_description desc("Formation Data Systems Configuration  Commands");
	desc.add_options()
	("help", "Show FDS  Configuration Options")
	("volume-create",po::value<std::string>(),"Create volume: volume-create <vol name> -s <size> -p <policy> -i <volume-id> ")
	("volume-delete",po::value<std::string>(), "Delete volume : volume-delete <vol name> -i <volume-id>")
	("volume-modify",po::value<std::string>(), "Modify volume: volume-modify <vol name> -s <size> -p <policy> -i <volume-id>")
	("volume-show",po::value<std::string>(), "Show volume")
	("ploicy-create",po::value<std::string>(), "Create Policy")
	("policy-delete",po::value<std::string>(), "Delete policy")
	("policy-modify",po::value<std::string>(), "Modify policy")
	("policy-show",po::value<std::string>(), "Show policy")
	("volume-size,s",po::value<double>(),"volume capacity")
	("volume-policy,p",po::value<int>(),"volume policy")
	("volume-id,i",po::value<int>(),"volume id");

	po::variables_map vm;

	po::store(po::parse_command_line(argc, argv, desc), vm) ;
	po::notify(vm);
	if (vm.count("help")) {
		std::system("clear");
		cout << "\n\n";
		cout << desc << "\n";
		return 1;
	}
	if (vm.count("volume-create") && vm.count("volume-size") && \
				 vm.count("volume-policy") && vm.count("volume-id")) {
		cout << vm["volume-create"].as<std::string>();
		cout << vm["volume-size"].as<double>();
		cout << vm["volume-policy"].as<int>();
		cout << vm["volume-id"].as<int>();

		FDSP_CreateVolTypePtr volData = new FDSP_CreateVolType();
		volData->vol_info = new FDSP_VolumeInfoType();

    		volData->vol_info->vol_name = vm["volume-create"].as<std::string>();
  		volData->vol_info->tennantId = 0;
  		volData->vol_info->localDomainId = 0;
  		volData->vol_info->globDomainId = 0;
    		volData->vol_info->volUUID = vm["volume-id"].as<int>();

    		volData->vol_info->capacity = vm["volume-size"].as<double>();
    		volData->vol_info->maxQuota = 0;
    		volData->vol_info->volType = FDSP_VOL_BLKDEV_TYPE;

  		volData->vol_info->replicaCnt = 0;
 	 	volData->vol_info->writeQuorum = 0;
 		volData->vol_info->readQuorum = 0;
    		volData->vol_info->consisProtocol = FDSP_CONS_PROTO_STRONG;

    		volData->vol_info->appWorkload = FDSP_APP_WKLD_TRANSACTION;

   		cfgPrx = FDSP_ConfigPathReqPrx::checkedCast(communicator()->stringToProxy (tcpProxyStr.str())); 
    		cfgPrx->CreateVol(msg_hdr, volData);


	} else if (vm.count("volume-modify") && vm.count("volume-size") &&  \
				vm.count("volume-policy") && vm.count("volume-id")) {
		cout << vm["volume-modify"].as<std::string>();
		cout << vm["volume-size"].as<double>();
		cout << vm["volume-policy"].as<int>();
		cout << vm["volume-id"].as<int>();

		FDSP_ModifyVolTypePtr volData = new FDSP_ModifyVolType();
		volData->vol_info = new FDSP_VolumeInfoType();

    		volData->vol_info->vol_name = vm["volume-modify"].as<std::string>();
    		volData->vol_info->volUUID = vm["volume-id"].as<int>();

    		volData->vol_info->capacity = vm["volume-size"].as<double>();
    		volData->vol_info->volType = FDSP_VOL_BLKDEV_TYPE;
    		volData->vol_info->consisProtocol = FDSP_CONS_PROTO_STRONG;
    		volData->vol_info->appWorkload = FDSP_APP_WKLD_TRANSACTION;

   		cfgPrx = FDSP_ConfigPathReqPrx::checkedCast(communicator()->stringToProxy (tcpProxyStr.str())); 
    		cfgPrx->ModifyVol(msg_hdr, volData);

	} else if(vm.count("volume-delete") && vm.count("volume-id")) {
		cout << vm["volume-delete"].as<std::string>();
		cout << vm["volume-id"].as<int>();

		FDSP_DeleteVolTypePtr volData = new FDSP_DeleteVolType();

    		volData->vol_name = vm["volume-delete"].as<std::string>();
    		volData->vol_uuid = vm["volume-id"].as<int>();
   		cfgPrx = FDSP_ConfigPathReqPrx::checkedCast(communicator()->stringToProxy (tcpProxyStr.str())); 
    		cfgPrx->DeleteVol(msg_hdr, volData);
	}

   return 0;
}


FdsCli::FdsCli() {
}

FdsCli::~FdsCli() {
}

   

  /*
   * Ice will execute the application via run()
   */
int FdsCli::run(int argc, char* argv[]) {

    /*
     * Setup the network communication connection with orch Manager. 
     */

    std::string orchMgrIPAddress;
    int orchMgrPortNum;

    Ice::PropertiesPtr props = communicator()->getProperties();

    orchMgrPortNum = props->getPropertyAsInt("OrchMgr.ConfigPort");
    orchMgrIPAddress = props->getProperty("OrchMgr.IPAddress");

    tcpProxyStr << "OrchMgr: tcp -h " << orchMgrIPAddress << " -p  " << orchMgrPortNum;
//    cfgPrx = FDSP_ControlPathReqPrx::checkedCast(communicator()->stringToProxy (tcpProxyStr.str())); 

    fdsCli->fdsCliPraser(argc, argv);
  }

}  // namespace fds

int main(int argc, char* argv[]) {

  fds::fdsCli = new fds::FdsCli();

  fds::fdsCli->main(argc, argv, "orch_mgr.conf");

  delete fds::fdsCli;
}
