/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <fdsCli.h>
#include <cli-policy.h>
#include <fds_assert.h>
#include <string>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <fds_process.h>

namespace fds {
FdsCli  *fdsCli;

FdsCli::FdsCli(int argc, char *argv[],
               const std::string &def_cfg_file,
               const std::string &base_path,
               const std::string &def_log_file, Module **mod_vec)
    : FdsProcess(argc, argv, def_cfg_file, base_path, def_log_file, mod_vec),
      my_node_name("fdscli")
{
    cli_log = g_fdslog;
    cli_log->setSeverityFilter(
        (fds_log::severity_level)conf_helper_.get<int>("log_severity"));
    FDS_PLOG(cli_log) << "Constructing the CLI";
}

FdsCli::~FdsCli() {
    FDS_PLOG(cli_log) << "Destructing the CLI";
}

void FdsCli::InitCfgMsgHdr(FDS_ProtocolInterface::FDSP_MsgHdrType* msg_hdr)
{
    fds_verify(msg_hdr);

    msg_hdr->minor_ver = 0;
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_REQ;
    msg_hdr->msg_id = 1;

    msg_hdr->major_ver = 0xa5;
    msg_hdr->minor_ver = 0x5a;

    msg_hdr->num_objects = 1;
    msg_hdr->frag_len = 0;
    msg_hdr->frag_num = 0;

    msg_hdr->tennant_id = 0;
    msg_hdr->local_domain_id = 0;

    msg_hdr->src_id = FDS_ProtocolInterface::FDSP_CLI_MGR;
    msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_ORCH_MGR;
    msg_hdr->src_node_name = "";

    msg_hdr->src_port = 0;
    msg_hdr->dst_port = 0;

    msg_hdr->err_code = ERR_OK;
    msg_hdr->result = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_msg = "";

    msg_hdr->req_cookie = 0;
    msg_hdr->msg_chksum = 0;  // ?
}

/* returns S3 type if un-recognized string */
FDS_ProtocolInterface::FDSP_VolType FdsCli::stringToVolType(
    const std::string& vol_type)
{
    if (vol_type == "s3") {
        return FDS_ProtocolInterface::FDSP_VOL_S3_TYPE;
    } else if (vol_type == "blk") {
        return FDS_ProtocolInterface::FDSP_VOL_BLKDEV_TYPE;
    }
    return FDS_ProtocolInterface::FDSP_VOL_S3_TYPE;
}

FDSP_ConfigPathReqClientPtr FdsCli::startClientSession() {
    netConfigPathClientSession *client_session =
            net_session_tbl->startSession<netConfigPathClientSession>(om_ip,
                                          om_cfg_port,
                                          FDS_ProtocolInterface::FDSP_ORCH_MGR,
                                          1,
                                          boost::shared_ptr<FDSP_ConfigPathRespIf>());
    fds_verify(client_session != NULL);
    return client_session->getClient();
}

void FdsCli::endClientSession() {
    net_session_tbl->endClientSession(om_ip, om_cfg_port);
}

int FdsCli::fdsCliParser(int argc, char* argv[])
{
    FDS_ProtocolInterface::FDSP_MsgHdrType msg_hdr;
    InitCfgMsgHdr(&msg_hdr);
    int return_code = 0;

    namespace po = boost::program_options;
    // Declare the supported options.
    po::options_description desc("Formation Data Systems Configuration  Commands");
    desc.add_options()
            ("help", "Show FDS  Configuration Options")
            ("volume-create", po::value<std::string>(),
             "Create volume: volume-create <vol name>"
             " -s <size> -p <policy-id> -i <volume-id> ")
            ("volume-delete", po::value<std::string>(),
             "Delete volume : volume-delete <vol name> -i <volume-id>")
            ("volume-modify", po::value<std::string>(),
             "Modify volume: volume-modify <vol name> -s <size> "
             "[-p <policy-id> OR -g <iops-min> -m <iops-max> -r <rel-prio>]")
            ("volume-attach", po::value<std::string>(),
             "Attach volume: volume-attach <vol name> -i <volume-id> -n <node-id>")
            ("volume-detach", po::value<std::string>(),
             "Detach volume: volume-detach <vol name> -i <volume-id> -n <node-id>")
            ("volume-show", po::value<std::string>(), "Show volume")
            ("policy-create", po::value<std::string>(),
             "Create Policy: policy-create <policy name> "
             "-p <policy-id> -g <iops-min> -m <iops-max> -r <rel-prio> ")
            ("policy-delete", po::value<std::string>(),
             "Delete policy: policy-delete <policy name> -p <policy-id>")
            ("policy-modify", po::value<std::string>(),
             "Modify policy: policy-modify <policy name> "
             "-p <policy-id> -g <iops-min> -m <iops-max> -r <rel-prio> ")
            ("domain-create", po::value<std::string>(),
             "Create domain: domain-create <domain name> -k <domain-id>")
            ("domain-delete", po::value<std::string>(),
             "Delete domain: domain-delete <domain name> -k <domain-id>")
            ("domain-stats", "Get domain stats: domain-stats -k <domain-id>")
            ("activate-nodes", po::value<std::string>(),
             "Activate discovered nodes: activate-nodes <domain name> -k <domain-id>"
             "[ -e \"am,dm,sm\" ]")
            ("throttle", "Throttle traffic: throttle -t <throttle_level> ")
            ("policy-show", po::value<std::string>(), "Show policy")
            ("volume-size,s", po::value<double>(), "volume capacity")
            ("volume-policy,p", po::value<int>(), "volume policy")
            ("node-id,n", po::value<std::string>(), "node id")
            ("volume-id,i", po::value<int>(), "volume id")
            ("domain-id,k", po::value<int>(), "domain id")
            ("throttle-level,t", po::value<float>(), "throttle level")
            ("remove-node,t", po::value<std::string>(), "remove node")
            ("iops-min,g", po::value<double>(), "minimum IOPS")
            ("iops-max,m", po::value<double>(), "maximum IOPS")
            ("rel-prio,r", po::value<int>(), "relative priority")
            ("enable-service,e", po::value<std::string>()->default_value("dm,sm,am"),
             "service to enable (if dm and sm, speficy \"dm,sm\")")
            ("vol-type,y", po::value<std::string>()->default_value("s3"),
             "volume access type")
            ("om_ip", po::value<std::string>(),
             "OM IP addr") /* Consumed already */
            ("om_port", po::value<fds_uint32_t>(),
             "OM config port"); /* Consumed already */

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).allow_unregistered().run(), vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << "\n\n";
        std::cout << desc << "\n";
        gl_OMCli.mod_run();
        return 1;
    }

    FDSP_ConfigPathReqClientPtr cfgPrx = startClientSession();

    if (vm.count("volume-create") && vm.count("volume-size") &&
        vm.count("volume-policy") && vm.count("volume-id")) {
        FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << "Constructing the CLI";
        FDS_PLOG_SEV(cli_log, fds::fds_log::notification) << " Create Volume ";
        FDS_PLOG_SEV(cli_log, fds::fds_log::notification)
                << vm["volume-create"].as<std::string>() << " -volume name";
        FDS_PLOG_SEV(cli_log, fds::fds_log::notification)
                << vm["volume-size"].as<double>() << " -volume size";
        FDS_PLOG_SEV(cli_log, fds::fds_log::notification)
                << vm["volume-policy"].as<int>() << " -volume policy";
        FDS_PLOG_SEV(cli_log, fds::fds_log::notification)
                << vm["volume-id"].as<int>() << " -volume id";

        FDS_ProtocolInterface::FDSP_CreateVolType volData;
        volData.vol_name = vm["volume-create"].as<std::string>();
        volData.vol_info.vol_name = vm["volume-create"].as<std::string>();
        volData.vol_info.tennantId = 0;
        volData.vol_info.localDomainId = 0;
        volData.vol_info.globDomainId = 0;

        volData.vol_info.capacity = vm["volume-size"].as<double>();
        volData.vol_info.maxQuota = 0;
        volData.vol_info.volType =
                stringToVolType(vm.count("vol-type") ?
                                vm["vol-type"].as<std::string>() : "");

        volData.vol_info.defReplicaCnt = 0;
        volData.vol_info.defWriteQuorum = 0;
        volData.vol_info.defReadQuorum = 0;
        volData.vol_info.defConsisProtocol =
                FDS_ProtocolInterface::FDSP_CONS_PROTO_STRONG;

        volData.vol_info.volPolicyId = vm["volume-policy"].as<int>();
        volData.vol_info.archivePolicyId = 0;
        volData.vol_info.placementPolicy = 0;
        volData.vol_info.appWorkload = FDS_ProtocolInterface::FDSP_APP_WKLD_TRANSACTION;

        if ((return_code = cfgPrx->CreateVol(msg_hdr, volData)) !=0) {
            std::system("clear");
            cout << "Error: Creating the Volume \n";
        }

    } else if (vm.count("volume-modify") && vm.count("volume-size")) {
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << " Modify Volume ";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["volume-modify"].as<std::string>() << " -volume name";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["volume-size"].as<double>() << " -volume size";
        if (vm.count("volume-policy")) {
            FDS_PLOG_SEV(cli_log, fds_log::notification)
                    << vm["volume-policy"].as<int>() << " -volume policy";
            if (vm.count("iops-min") || vm.count("iops-max") || vm.count("rel-prio")) {
                FDS_PLOG_SEV(cli_log, fds_log::notification)
                        << "Since prolicy id is specified, "
                        << "min/max iops and priority will be ignored";
            }
        } else {
            if ((vm.count("iops-min") == 0) ||
                (vm.count("iops-max") == 0) ||
                (vm.count("rel-prio") == 0)) {
                FDS_PLOG_SEV(cli_log, fds::fds_log::error)
                        << "If 'volume-policy' is not specified, "
                        << "must specify min/max iops and relative priority";
                return 0;
            }
            FDS_PLOG_SEV(cli_log, fds_log::notification)
                    << vm["iops-min"].as<double>() << " -minimum iops";
            FDS_PLOG_SEV(cli_log, fds_log::notification)
                    << vm["iops-max"].as<double>() << " -maximum iops";
            FDS_PLOG_SEV(cli_log, fds_log::notification)
                    << vm["rel-prio"].as<int>() << " -relative priority";
        }

        FDS_ProtocolInterface::FDSP_ModifyVolType volData;
        volData.vol_name = vm["volume-modify"].as<std::string>();
        volData.vol_desc.vol_name = volData.vol_name;

        volData.vol_desc.capacity = vm["volume-size"].as<double>();
        volData.vol_desc.volType =
                stringToVolType(vm.count("vol-type") ?
                                vm["vol-type"].as<std::string>() : "");

        volData.vol_desc.defConsisProtocol =
                FDS_ProtocolInterface::FDSP_CONS_PROTO_STRONG;
        volData.vol_desc.archivePolicyId = 0;
        volData.vol_desc.placementPolicy = 0;
        volData.vol_desc.appWorkload = FDS_ProtocolInterface::FDSP_APP_WKLD_TRANSACTION;

        if (vm.count("volume-policy")) {
            volData.vol_desc.volPolicyId = vm["volume-policy"].as<int>();
        } else {
            volData.vol_desc.volPolicyId = 0; /* 0 means don't modify actual policy id */
            volData.vol_desc.iops_min = vm["iops-min"].as<double>();
            volData.vol_desc.iops_max = vm["iops-max"].as<double>();
            volData.vol_desc.rel_prio = vm["rel-prio"].as<int>();
        }

        cfgPrx->ModifyVol(msg_hdr, volData);

    } else if (vm.count("volume-delete") && vm.count("volume-id")) {
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << " Delete Volume ";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["volume-delete"].as<std::string>() << " -volume name";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["volume-id"].as<int>() << " -volume id";

        FDS_ProtocolInterface::FDSP_DeleteVolType volData;
        volData.vol_name = vm["volume-delete"].as<std::string>();

        cfgPrx->DeleteVol(msg_hdr, volData);

    } else if (vm.count("volume-attach") &&
               vm.count("volume-id") &&
               vm.count("node-id")) {
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << " Attach Volume ";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["volume-attach"].as<std::string>() << " -volume name";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["volume-id"].as<int>() << " -volume id";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["node-id"].as<std::string>() << " -node id";

        FDS_ProtocolInterface::FDSP_AttachVolCmdType volData;
        volData.vol_name = vm["volume-attach"].as<std::string>();
        volData.node_id = vm["node-id"].as<std::string>();
        msg_hdr.src_node_name = vm["node-id"].as<std::string>();

        cfgPrx->AttachVol(msg_hdr, volData);

    } else if (vm.count("volume-detach") &&
               vm.count("volume-id") &&
               vm.count("node-id")) {
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << " Detach Volume ";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["volume-detach"].as<std::string>() << " -volume name";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["volume-id"].as<int>() << " -volume id";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["node-id"].as<std::string>() << " -node id";

        FDS_ProtocolInterface::FDSP_AttachVolCmdType volData;
        volData.vol_name = vm["volume-detach"].as<std::string>();
        volData.node_id = vm["node-id"].as<std::string>();
        msg_hdr.src_node_name = vm["node-id"].as<std::string>();

        cfgPrx->DetachVol(msg_hdr, volData);

    } else if ( vm.count("policy-create") && vm.count("volume-policy") && \
                vm.count("iops-max") && vm.count("iops-max") && vm.count("rel-prio")) {
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << " Create Policy ";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["policy-create"].as<std::string>() << " -policy name";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["volume-policy"].as<int>() << " -policy id";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["iops-min"].as<double>() << " -minimum iops";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["iops-max"].as<double>() << " -maximum iops";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["rel-prio"].as<int>() << " -relative priority";

        FDS_ProtocolInterface::FDSP_CreatePolicyType policyData;
        policyData.policy_name = vm["policy-create"].as<std::string>();

        policyData.policy_info.policy_name = vm["policy-create"].as<std::string>();
        policyData.policy_info.policy_id = vm["volume-policy"].as<int>();
        policyData.policy_info.iops_min = vm["iops-min"].as<double>();
        policyData.policy_info.iops_max = vm["iops-max"].as<double>();
        policyData.policy_info.rel_prio = vm["rel-prio"].as<int>();

        cfgPrx->CreatePolicy(msg_hdr, policyData);

    } else if (vm.count("policy-delete") && vm.count("volume-policy")) {
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << " Delete Policy ";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["policy-delete"].as<std::string>() << " -policy name";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["volume-policy"].as<int>() << " -policy id";

        FDS_ProtocolInterface::FDSP_DeletePolicyType policyData;

        policyData.policy_name = vm["policy-delete"].as<std::string>();
        policyData.policy_id = vm["volume-policy"].as<int>();

        cfgPrx->DeletePolicy(msg_hdr, policyData);

    } else if (vm.count("policy-modify") && vm.count("volume-policy") && \
               vm.count("iops-max") && vm.count("iops-max") && vm.count("rel-prio")) {
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << " Modify Policy ";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["policy-modify"].as<std::string>() << " -policy name";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["volume-policy"].as<int>() << " -policy id";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["iops-min"].as<double>() << " -minimum iops";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["iops-max"].as<double>() << " -maximum iops";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["rel-prio"].as<int>() << " -relative priority";

        FDS_ProtocolInterface::FDSP_ModifyPolicyType policyData;
        policyData.policy_name = vm["policy-modify"].as<std::string>();

        policyData.policy_info.policy_name = vm["policy-modify"].as<std::string>();
        policyData.policy_info.policy_id = vm["volume-policy"].as<int>();
        policyData.policy_info.iops_min = vm["iops-min"].as<double>();
        policyData.policy_info.iops_max = vm["iops-max"].as<double>();
        policyData.policy_info.rel_prio = vm["rel-prio"].as<int>();

        cfgPrx->ModifyPolicy(msg_hdr, policyData);

    } else if (vm.count("domain-create") && vm.count("domain-id")) {
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << " Domain Create ";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["domain-create"].as<std::string>() << "-domain name";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["domain-id"].as<int>() <<  " -domain id ";

        FDS_ProtocolInterface::FDSP_CreateDomainType domainData;
        domainData.domain_name = vm["domain-create"].as<std::string>();
        domainData.domain_id = vm["domain-id"].as<int>();

        cfgPrx->CreateDomain(msg_hdr, domainData);
    }  else if (vm.count("remove-node")) {
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << " Remove Node ";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["remove-node"].as<std::string>() << "- node name";
        FDS_ProtocolInterface::FDSP_RemoveNodeType removeNodeData;
        removeNodeData.node_name = vm["remove-node"].as<std::string>();

        cfgPrx->RemoveNode(msg_hdr, removeNodeData);
    }  else if (vm.count("activate-nodes")) {
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << " Activate Nodes: domain name "
                << vm["activate-nodes"].as<std::string>()
                << " (domain name ignored for now, using default domain)"
                << " and enable services: "
                << vm["enable-service"].as<std::string>();
        FDS_ProtocolInterface::FDSP_ActivateAllNodesType activateNodesData;
        activateNodesData.domain_id = vm["domain-id"].as<int>();
        activateNodesData.activate_sm = false;
        activateNodesData.activate_dm = false;
        activateNodesData.activate_am = false;
        std::string svc_str = vm["enable-service"].as<std::string>();
        boost::char_separator<char> sep(",");
        boost::tokenizer<boost::char_separator<char>> tokens(svc_str, sep);
        for (const auto& t : tokens) {
            if (t.compare("am") == 0) {
                activateNodesData.activate_am = true;
                FDS_PLOG(cli_log) << "   -- activate AM";
            } else if (t.compare("sm") == 0) {
                activateNodesData.activate_sm = true;
                FDS_PLOG(cli_log) << "   -- activate SM";
            } else if (t.compare("dm") == 0) {
                activateNodesData.activate_dm = true;
                FDS_PLOG(cli_log) << "   -- activate DM";
            } else {
                cout << "Unknown service " << t << ", will ignore" << std::endl;
                FDS_PLOG_SEV(cli_log, fds_log::warning)
                        << "Unknown service " << t << "is specified; "
                        << "will ignore, but check your fdscli command";
            }
        }

        cfgPrx->ActivateAllNodes(msg_hdr, activateNodesData);
    }  else if (vm.count("domain-delete") && vm.count("domain-id")) {
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << " Domain Delete ";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["domain-delete"].as<std::string>() << "-domain name";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["domain-id"].as<int>() <<  " -domain id ";

        FDS_ProtocolInterface::FDSP_CreateDomainType domainData;
        domainData.domain_name = vm["domain-delete"].as<std::string>();
        domainData.domain_id = vm["domain-id"].as<int>();

        cfgPrx->DeleteDomain(msg_hdr, domainData);
    } else if (vm.count("domain-stats") && vm.count("domain-id")) {
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << " Domain Stats ";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["domain-id"].as<int>() <<  " -domain id ";

        FDS_ProtocolInterface::FDSP_GetDomainStatsType domainData;
        domainData.domain_id = vm["domain-id"].as<int>();

        cfgPrx->GetDomainStats(msg_hdr, domainData);

    } else if (vm.count("throttle") && vm.count("throttle-level")) {
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << " Throttle ";
        FDS_PLOG_SEV(cli_log, fds_log::notification)
                << vm["throttle-level"].as<float>() << "-throttle_level";

        FDS_ProtocolInterface::FDSP_ThrottleMsgType throttle_msg;
        throttle_msg.domain_id = 0;
        throttle_msg.throttle_level = vm["throttle-level"].as<float>();

        cfgPrx->SetThrottleLevel(msg_hdr, throttle_msg);
    } else {
        gl_OMCli.setCliClient(cfgPrx);
        gl_OMCli.mod_run();
    }

    endClientSession();
    return 0;
}

int FdsCli::run(int argc, char* argv[])
{
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
    if (omConfigPort == 0) {
        omConfigPort = conf_helper_.get<int>("config_port");
    }
    if (omIpStr.empty() == true) {
        omIpStr = conf_helper_.get<std::string>("ip_address");
    }

    /* remember port and ip for getting client later */
    om_cfg_port = omConfigPort;
    om_ip = netSession::ipString2Addr(omIpStr);

    net_session_tbl = boost::shared_ptr<netSessionTbl>(
        new netSessionTbl(my_node_name,
                          om_ip,
                          om_cfg_port,
                          10,
                          FDS_ProtocolInterface::FDSP_CLI_MGR));

    fdsCli->fdsCliParser(argc, argv);
    return 0;
}

}  // namespace fds

int main(int argc, char* argv[])
{
    fds::Module *cliVec[] = {
        &fds::gl_OMCli,
        nullptr
    };
    fds::fdsCli = new fds::FdsCli(argc, argv,
                                  "orch_mgr.conf", "fds.om.", "cli.log", cliVec);

    fds::fdsCli->setup();
    fds::fdsCli->run(argc, argv);
    delete fds::fdsCli;
    return 0;
}

