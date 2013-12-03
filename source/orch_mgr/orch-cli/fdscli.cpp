/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include "fdsCli.h"
#include "cli-policy.h"
#include <fds_assert.h>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>

using namespace std;
using namespace FDS_ProtocolInterface;

namespace fds {
FdsCli  *fdsCli;
std::ostringstream tcpProxyStr;
FDSP_ConfigPathReqPrx  cfgPrx;

FdsCli::FdsCli() {
  cli_log = new fds_log("cli", "logs");
  FDS_PLOG(cli_log) << "Constructing the CLI";
}

FdsCli::~FdsCli() {
  FDS_PLOG(cli_log) << "Destructing the CLI";
  delete cli_log;
}

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
	msg_hdr->src_node_name = "";
	
	msg_hdr->src_port = 0;
	msg_hdr->dst_port = 0;

        msg_hdr->err_code=FDSP_ERR_SM_NO_SPACE;
        msg_hdr->result=FDSP_ERR_OK;

}


int FdsCli::fdsCliPraser(int argc, char* argv[])
{
	FDSP_MsgHdrTypePtr msg_hdr = new FDSP_MsgHdrType;
	InitCfgMsgHdr(msg_hdr);
	int return_code=0;

	namespace po = boost::program_options;
	// Declare the supported options.
        po::options_description desc("Formation Data Systems Configuration  Commands");
	desc.add_options()
	("help", "Show FDS  Configuration Options")
	("volume-create",po::value<std::string>(),"Create volume: volume-create <vol name> -s <size> -p <policy-id> -i <volume-id> ")
	("volume-delete",po::value<std::string>(), "Delete volume : volume-delete <vol name> -i <volume-id>")
	("volume-modify",po::value<std::string>(), "Modify volume: volume-modify <vol name> -s <size> [-p <policy-id> OR -g <iops-min> -m <iops-max> -r <rel-prio>]")
	("volume-attach",po::value<std::string>(), "Attach volume: volume-attach <vol name> -i <volume-id> -n <node-id>")
	("volume-detach",po::value<std::string>(), "Detach volume: volume-detach <vol name> -i <volume-id> -n <node-id>")
	("volume-show",po::value<std::string>(), "Show volume")
	("policy-create",po::value<std::string>(), "Create Policy: policy-create <policy name> -p <policy-id> -g <iops-min> -m <iops-max> -r <rel-prio> ")
	("policy-delete",po::value<std::string>(), "Delete policy: policy-delete <policy name> -p <policy-id>")
	("policy-modify",po::value<std::string>(), "Modify policy: policy-modify <policy name> -p <policy-id> -g <iops-min> -m <iops-max> -r <rel-prio> ")
	("domain-create",po::value<std::string>(), "Create domain: domain-create <domain name> -k <domain-id>")
	("domain-delete",po::value<std::string>(), "Delete domain: domain-delete <domain name> -k <domain-id>")
	("domain-stats", "Get domain stats: domain-stats -k <domain-id>")
	("throttle", "Throttle traffic: throttle -t <throttle_level> ")
	("policy-show",po::value<std::string>(), "Show policy")
	("volume-size,s",po::value<double>(),"volume capacity")
	("volume-policy,p",po::value<int>(),"volume policy")
	("node-id,n",po::value<std::string>(),"node id")
	("volume-id,i",po::value<int>(),"volume id")
	("domain-id,k",po::value<int>(),"domain id")
	("throttle-level,t", po::value<float>(), "throttle level")
	("iops-min,g",po::value<double>(),"minimum IOPS")
	("iops-max,m",po::value<double>(),"maximum IOPS")
	("rel-prio,r",po::value<int>(),"relative priority")
        ("vol-type,y",po::value<std::string>()->default_value("s3"),"volume access type")
    ("om_ip", po::value<std::string>(), "OM IP addr") /* Consumed already */
    ("om_port", po::value<fds_uint32_t>(), "OM config port"); /* Consumed already */

	po::variables_map vm;
    Ice::ObjectPrx proxy = communicator()->stringToProxy(tcpProxyStr.str());

    po::store(po::command_line_parser(argc, argv).
            options(desc).allow_unregistered().run(), vm);
	po::notify(vm);
	if (vm.count("help")) {
		cout << "\n\n";
		cout << desc << "\n";
        gl_OMCli.mod_run();
		return 1;
	}
	if (vm.count("volume-create") && vm.count("volume-size") && \
				 vm.count("volume-policy") && vm.count("volume-id")) {

  FDS_PLOG(cli_log) << "Constructing the CLI";
		FDS_PLOG(cli_log) << " Create Volume ";
		FDS_PLOG(cli_log) << vm["volume-create"].as<std::string>() << " -volume name";
		FDS_PLOG(cli_log) << vm["volume-size"].as<double>() << " -volume size";
		FDS_PLOG(cli_log) << vm["volume-policy"].as<int>() << " -volume policy";
		FDS_PLOG(cli_log) << vm["volume-id"].as<int>() << " -volume id";

		FDSP_CreateVolTypePtr volData = new FDSP_CreateVolType();
		volData->vol_info = new FDSP_VolumeInfoType();

    		volData->vol_info->vol_name = vm["volume-create"].as<std::string>();
  		volData->vol_info->tennantId = 0;
  		volData->vol_info->localDomainId = 0;
  		volData->vol_info->globDomainId = 0;
    		//volData->vol_info->volUUID = vm["volume-id"].as<int>();

    		volData->vol_info->capacity = vm["volume-size"].as<double>();
    		volData->vol_info->maxQuota = 0;
                if (vm.count("vol-type")) {
                  if (vm["vol-type"].as<std::string>() == "s3") {
                    volData->vol_info->volType = FDSP_VOL_S3_TYPE;
                  } else {
                    fds_verify(vm["vol-type"].as<std::string>() == "blk");
                    volData->vol_info->volType = FDSP_VOL_BLKDEV_TYPE;
                  }
                } else {
                  volData->vol_info->volType = FDSP_VOL_S3_TYPE;
                }

  		volData->vol_info->defReplicaCnt = 0;
 	 	volData->vol_info->defWriteQuorum = 0;
 		volData->vol_info->defReadQuorum = 0;
    		volData->vol_info->defConsisProtocol = FDSP_CONS_PROTO_STRONG;

		volData->vol_info->volPolicyId = vm["volume-policy"].as<int>();
		volData->vol_info->archivePolicyId = 0;
		volData->vol_info->placementPolicy = 0;
    		volData->vol_info->appWorkload = FDSP_APP_WKLD_TRANSACTION;

   		cfgPrx = FDSP_ConfigPathReqPrx::checkedCast(proxy);
    		if((return_code = cfgPrx->CreateVol(msg_hdr, volData)) !=0 ) {
		  std::system("clear");
		  cout << "Error: Creating the Volume, Running out of Disk  resources \n";
		}


	} else if (vm.count("volume-modify") && vm.count("volume-size")) {

	  FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << " Modify Volume ";
	  FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << vm["volume-modify"].as<std::string>() << " -volume name";
	  FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << vm["volume-size"].as<double>() << " -volume size";
	  if (vm.count("volume-policy")) {
	    FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << vm["volume-policy"].as<int>() << " -volume policy";
	    if (vm.count("iops-min") || vm.count("iops-max") || vm.count("rel-prio")) {
	      FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << "Since prolicy id is specified, min/max iops and priority will be ignored";
	    }

	  }
	  else {
	    if ((vm.count("iops-min")==0) || (vm.count("iops-max")==0) || (vm.count("rel-prio")==0)) {
	      FDS_PLOG_SEV(cli_log, fds::fds_log::error) << "If 'volume-policy' is not specified, must specify min/max iops and relative priority";
	      return 0;
	    }
	    FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << vm["iops-min"].as<double>() << " -minimum iops";		
	    FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << vm["iops-max"].as<double>() << " -maximum iops";
	    FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << vm["rel-prio"].as<int>() << " -relative priority";
	  }

		FDSP_ModifyVolTypePtr volData = new FDSP_ModifyVolType();
		volData->vol_desc = new FDSP_VolumeDescType();

    		volData->vol_name = vm["volume-modify"].as<std::string>();
    		volData->vol_desc->vol_name = volData->vol_name;

    		volData->vol_desc->capacity = vm["volume-size"].as<double>();
                if (vm.count("vol-type")) {
                  if (vm["vol-type"].as<std::string>() == "s3") {
                    volData->vol_desc->volType = FDSP_VOL_S3_TYPE;
                  } else {
                    fds_verify(vm["vol-type"].as<std::string>() == "blk");
                    volData->vol_desc->volType = FDSP_VOL_BLKDEV_TYPE;
                  }
                } else {
                  volData->vol_desc->volType = FDSP_VOL_S3_TYPE;
                }
    		volData->vol_desc->defConsisProtocol = FDSP_CONS_PROTO_STRONG;

		volData->vol_desc->archivePolicyId = 0;
		volData->vol_desc->placementPolicy = 0;
    		volData->vol_desc->appWorkload = FDSP_APP_WKLD_TRANSACTION;

	        if (vm.count("volume-policy")) {
		  volData->vol_desc->volPolicyId = vm["volume-policy"].as<int>();
		}
		else { 
		  volData->vol_desc->volPolicyId = 0; /* 0 means don't modify actual policy id */
		  volData->vol_desc->iops_min = vm["iops-min"].as<double>();
		  volData->vol_desc->iops_max = vm["iops-max"].as<double>();
		  volData->vol_desc->rel_prio = vm["rel-prio"].as<int>();
		}

   		cfgPrx = FDSP_ConfigPathReqPrx::checkedCast(proxy);
    		cfgPrx->ModifyVol(msg_hdr, volData);

	} else if(vm.count("volume-delete") && vm.count("volume-id")) {

		FDS_PLOG(cli_log) << " Delete Volume ";
		FDS_PLOG(cli_log) << vm["volume-delete"].as<std::string>() << " -volume name";
		FDS_PLOG(cli_log) << vm["volume-id"].as<int>() << " -volume id";

		FDSP_DeleteVolTypePtr volData = new FDSP_DeleteVolType();

    		volData->vol_name = vm["volume-delete"].as<std::string>();
    		//volData->vol_uuid = vm["volume-id"].as<int>();
   		cfgPrx = FDSP_ConfigPathReqPrx::checkedCast(proxy);
    		cfgPrx->DeleteVol(msg_hdr, volData);
	} else if(vm.count("volume-attach") && vm.count("volume-id") && vm.count("node-id")) {

		FDS_PLOG(cli_log) << " Attach Volume ";
		FDS_PLOG(cli_log) << vm["volume-attach"].as<std::string>() << " -volume name";
		FDS_PLOG(cli_log) << vm["volume-id"].as<int>() << " -volume id";
		FDS_PLOG(cli_log) << vm["node-id"].as<std::string>() << " -node id";

		FDSP_AttachVolCmdTypePtr volData = new FDSP_AttachVolCmdType();

    		volData->vol_name = vm["volume-attach"].as<std::string>();
    		// volData->vol_uuid = vm["volume-id"].as<int>();
    		volData->node_id = vm["node-id"].as<std::string>();
                msg_hdr->src_node_name = vm["node-id"].as<std::string>();
   		cfgPrx = FDSP_ConfigPathReqPrx::checkedCast(proxy);
    		cfgPrx->AttachVol(msg_hdr, volData);

	} else if(vm.count("volume-detach") && vm.count("volume-id") && vm.count("node-id")) {

		FDS_PLOG(cli_log) << " Detach Volume ";
		FDS_PLOG(cli_log) << vm["volume-detach"].as<std::string>() << " -volume name";
		FDS_PLOG(cli_log) << vm["volume-id"].as<int>() << " -volume id";
		FDS_PLOG(cli_log) << vm["node-id"].as<std::string>() << " -node id";

		FDSP_AttachVolCmdTypePtr volData = new FDSP_AttachVolCmdType();

    		volData->vol_name = vm["volume-detach"].as<std::string>();
    		//volData->vol_uuid = vm["volume-id"].as<int>();
    		volData->node_id = vm["node-id"].as<std::string>();
   		cfgPrx = FDSP_ConfigPathReqPrx::checkedCast(proxy);
    		cfgPrx->DetachVol(msg_hdr, volData);

	} else 	if (vm.count("policy-create") && vm.count("volume-policy") && \
		    vm.count("iops-max") && vm.count("iops-max") && vm.count("rel-prio")) {

		FDS_PLOG(cli_log) << " Create Policy ";
		FDS_PLOG(cli_log) << vm["policy-create"].as<std::string>() << " -policy name";
		FDS_PLOG(cli_log) << vm["volume-policy"].as<int>() << " -policy id";
		FDS_PLOG(cli_log) << vm["iops-min"].as<double>() << " -minimum iops";
		FDS_PLOG(cli_log) << vm["iops-max"].as<double>() << " -maximum iops";
		FDS_PLOG(cli_log) << vm["rel-prio"].as<int>() << " -relative priority";

		FDSP_CreatePolicyTypePtr policyData = new FDSP_CreatePolicyType();
		policyData->policy_info = new FDSP_PolicyInfoType();

    		policyData->policy_info->policy_name = vm["policy-create"].as<std::string>();
  		policyData->policy_info->policy_id = vm["volume-policy"].as<int>();
  		policyData->policy_info->iops_min = vm["iops-min"].as<double>();
  		policyData->policy_info->iops_max = vm["iops-max"].as<double>();
    		policyData->policy_info->rel_prio = vm["rel-prio"].as<int>();
   		cfgPrx = FDSP_ConfigPathReqPrx::checkedCast(proxy);
    		cfgPrx->CreatePolicy(msg_hdr, policyData);

	} else if(vm.count("policy-delete") && vm.count("volume-policy")) {

		FDS_PLOG(cli_log) << " Delete Policy ";
		FDS_PLOG(cli_log) << vm["policy-delete"].as<std::string>() << " -policy name";
		FDS_PLOG(cli_log) << vm["volume-policy"].as<int>() << " -policy id";

		FDSP_DeletePolicyTypePtr policyData = new FDSP_DeletePolicyType();

    		policyData->policy_name = vm["policy-delete"].as<std::string>();
    		policyData->policy_id = vm["volume-policy"].as<int>();
   		cfgPrx = FDSP_ConfigPathReqPrx::checkedCast(proxy);
    		cfgPrx->DeletePolicy(msg_hdr, policyData);

	} else 	if (vm.count("policy-modify") && vm.count("volume-policy") && \
		    vm.count("iops-max") && vm.count("iops-max") && vm.count("rel-prio")) {

		FDS_PLOG(cli_log) << " Modify Policy ";
		FDS_PLOG(cli_log) << vm["policy-modify"].as<std::string>() << " -policy name";
		FDS_PLOG(cli_log) << vm["volume-policy"].as<int>() << " -policy id";
		FDS_PLOG(cli_log) << vm["iops-min"].as<double>() << " -minimum iops";
		FDS_PLOG(cli_log) << vm["iops-max"].as<double>() << " -maximum iops";
		FDS_PLOG(cli_log) << vm["rel-prio"].as<int>() << " -relative priority";

		FDSP_ModifyPolicyTypePtr policyData = new FDSP_ModifyPolicyType();
		policyData->policy_info = new FDSP_PolicyInfoType();

    		policyData->policy_info->policy_name = vm["policy-modify"].as<std::string>();
  		policyData->policy_info->policy_id = vm["volume-policy"].as<int>();
  		policyData->policy_info->iops_min = vm["iops-min"].as<double>();
  		policyData->policy_info->iops_max = vm["iops-max"].as<double>();
    		policyData->policy_info->rel_prio = vm["rel-prio"].as<int>();
   		cfgPrx = FDSP_ConfigPathReqPrx::checkedCast(proxy);
    		cfgPrx->ModifyPolicy(msg_hdr, policyData);

	} else 	if (vm.count("domain-create") && vm.count("domain-id")) {
		FDS_PLOG(cli_log) << " Domain Create ";
		FDS_PLOG(cli_log) << vm["domain-create"].as<std::string>() << "-domain name";
		FDS_PLOG(cli_log) << vm["domain-id"].as<int>() <<  " -domain id ";
		
		FDSP_CreateDomainTypePtr domainData = new FDSP_CreateDomainType();
		domainData->domain_name = vm["domain-create"].as<std::string>();
		domainData->domain_id = vm["domain-id"].as<int>();
   		cfgPrx = FDSP_ConfigPathReqPrx::checkedCast(proxy);
    		cfgPrx->CreateDomain(msg_hdr, domainData);

	} else 	if (vm.count("domain-delete") && vm.count("domain-id")) {
	  FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << " Domain Delete ";
	  FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << vm["domain-delete"].as<std::string>() << "-domain name";
	  FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << vm["domain-id"].as<int>() <<  " -domain id ";
		
		FDSP_CreateDomainTypePtr domainData = new FDSP_CreateDomainType();
		domainData->domain_name = vm["domain-delete"].as<std::string>();
		domainData->domain_id = vm["domain-id"].as<int>();
   		cfgPrx = FDSP_ConfigPathReqPrx::checkedCast(proxy);
    		cfgPrx->DeleteDomain(msg_hdr, domainData);
	} else 	if (vm.count("domain-stats") && vm.count("domain-id")) {
	  FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << " Domain Stats ";
	  FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << vm["domain-id"].as<int>() <<  " -domain id ";
		
	  FDSP_GetDomainStatsTypePtr domainData = new FDSP_GetDomainStatsType();
	  domainData->domain_id = vm["domain-id"].as<int>();
	  cfgPrx = FDSP_ConfigPathReqPrx::checkedCast(proxy);
	  cfgPrx->GetDomainStats(msg_hdr, domainData);
        } else if (vm.count("throttle") && vm.count("throttle-level")) {
	  FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << " Throttle ";
	  FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << vm["throttle-level"].as<float>() << "-throttle_level";

		FDSP_ThrottleMsgTypePtr throttle_msg = new FDS_ProtocolInterface::FDSP_ThrottleMsgType;
		throttle_msg->domain_id = 0;
		throttle_msg->throttle_level = vm["throttle-level"].as<float>();
		cfgPrx = FDSP_ConfigPathReqPrx::checkedCast(proxy);
		cfgPrx->SetThrottleLevel(msg_hdr, throttle_msg);
	} else {
        gl_OMCli.mod_run();
    }

   return 0;
}


  /*
   * Ice will execute the application via run()
   */
int FdsCli::run(int argc, char* argv[]) {
  /*
   * Process the cmdline args.
   */
  fds_uint32_t omConfigPort = 0;
  std::string  omIpStr;
  for (fds_int32_t i = 1; i < argc; i++) {
    if (strncmp(argv[i], "--om_ip=", 8) == 0) {
      omIpStr = argv[i] + 8;
    } else if (strncmp(argv[i], "--om_port=", 10) == 0) {
      omConfigPort = strtoul(argv[i] + 10, NULL, 0);
    }
  }

  /*
   * Setup the network communication connection with orch Manager. 
   */
  
  // std::string orchMgrIPAddress;
  // int orchMgrPortNum;
  
  Ice::PropertiesPtr props = communicator()->getProperties();
  
  if (omConfigPort == 0) {
    omConfigPort = props->getPropertyAsInt("OrchMgr.ConfigPort");
  }
  if (omIpStr.empty() == true) {
    omIpStr = props->getProperty("OrchMgr.IPAddress");
  }
  
  tcpProxyStr << "OrchMgr: tcp -h " << omIpStr << " -p  " << omConfigPort;
  
  fdsCli->fdsCliPraser(argc, argv);
}

}  // namespace fds

int main(int argc, char* argv[])
{
    fds::Module *cli_vec[] = {
        &fds::gl_OMCli,
        nullptr
    };
    fds::ModuleVector  fds_cli_vec(argc, argv, cli_vec);
    fds::fdsCli = new fds::FdsCli();

    fds_cli_vec.mod_execute();
    fds::fdsCli->main(argc, argv, "orch_mgr.conf");

    delete fds::fdsCli;
    return 0;
}

