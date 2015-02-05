/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <fdsCli.h>
#include <cli-policy.h>
#include <fds_assert.h>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <fds_process.h>

#define NETWORKCHECK(expr)                                     \
    try {                                                      \
        expr ;                                                 \
    } catch(const att::TTransportException& e) {               \
        LOGERROR << "error during network call : " << e.what();\
        cerr     << "error during network call : " << e.what();\
        return 1;                                              \
    }


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
        fds_log::getLevelFromName(conf_helper_.get<std::string>("log_severity")));
    LOGNORMAL << "Constructing the CLI";
}

FdsCli::~FdsCli() {
    LOGNORMAL << "Destructing the CLI";
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

/* returns hdd type if un-recognized string */
FDS_ProtocolInterface::FDSP_MediaPolicy FdsCli::stringToMediaPolicy(
    const std::string& media_policy)
{
    if (media_policy == "hdd") {
        return FDS_ProtocolInterface::FDSP_MEDIA_POLICY_HDD;
    } else if (media_policy == "ssd") {
        return FDS_ProtocolInterface::FDSP_MEDIA_POLICY_SSD;
    } else if (media_policy == "hybrid") {
        return FDS_ProtocolInterface::FDSP_MEDIA_POLICY_HYBRID;
    } else if (media_policy == "hybrid-prefcap") {
        return FDS_ProtocolInterface::FDSP_MEDIA_POLICY_HYBRID_PREFCAP;
    }
    return FDS_ProtocolInterface::FDSP_MEDIA_POLICY_HDD;
}

std::string FdsCli::mediaPolicyToString(
    const FDS_ProtocolInterface::FDSP_MediaPolicy media_policy)
{
    if (media_policy == FDS_ProtocolInterface::FDSP_MEDIA_POLICY_HDD) {
        return "hdd";
    } else if (media_policy == FDS_ProtocolInterface::FDSP_MEDIA_POLICY_SSD) {
        return "ssd";
    } else if (media_policy == FDS_ProtocolInterface::FDSP_MEDIA_POLICY_HYBRID) {
        return "hybrid";
    } else if (media_policy == FDS_ProtocolInterface::FDSP_MEDIA_POLICY_HYBRID_PREFCAP) {
        return "hybrid-prefcap";
    }
    return "unknown";
}

FDS_ProtocolInterface::FDSP_ScavengerCmd FdsCli::stringToScavengerCommand(
    const std::string& cmd)
{
    if (cmd == "start") {
        return FDS_ProtocolInterface::FDSP_SCAVENGER_START;
    } else if (cmd == "stop") {
        return FDS_ProtocolInterface::FDSP_SCAVENGER_STOP;
    } else if (cmd == "enable") {
        return FDS_ProtocolInterface::FDSP_SCAVENGER_ENABLE;
    } else if (cmd == "disable") {
        return FDS_ProtocolInterface::FDSP_SCAVENGER_DISABLE;
    }
    fds_verify(false);
}


FDSP_ConfigPathReqClientPtr FdsCli::startClientSession() {
    netConfigPathClientSession *client_session =
            net_session_tbl->
            startSession
            <netConfigPathClientSession>(om_ip,
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
            ("volume-get", po::value<std::string>(),
             "Get volume info: volume-get <vol name>")
            ("volume-attach", po::value<std::string>(),
             "Attach volume: volume-attach <vol name> -i <volume-id> -n <node-id>")
            ("volume-detach", po::value<std::string>(),
             "Detach volume: volume-detach <vol name> -i <volume-id> -n <node-id>")
            ("volume-show", po::value<std::string>(), "Show volume")
            ("list-volumes", po::value<std::string>(),
             "List volumes: list-volumes <domain_name> (domain name ignored for now")
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
            ("domain-shutdown", po::value<std::string>(),
             "Delete domain: domain-shutdown <domain name> -k <domain-id>")
            ("domain-stats", "Get domain stats: domain-stats -k <domain-id>")
            ("activate-nodes", po::value<std::string>(),
             "Activate discovered nodes: activate-nodes <domain name> -k <domain-id>"
             "[ -e \"am,dm,sm\" ]")
            ("activate-services", po::value<std::string>(),
             "Activate nodes: activate-services <domain> -k <domain-id> -w <node-uuid>"
             "[ -e \"am,dm,sm\" ]")
            ("list-services", "List Services")
            ("remove-services", po::value<std::string>(),
             "Remove services: remove-services <node_name> "
             "[ -e \"am,dm,sm\" ]")
            ("throttle", "Throttle traffic: throttle -t <throttle_level> ")
            ("scavenger", po::value<std::string>(),
             "scavenger enable|disable|start|stop")
            ("policy-show", po::value<std::string>(), "Show policy")
            ("volume-size,s", po::value<double>(), "volume capacity")
            ("volume-policy,p", po::value<int>(), "volume policy")
            ("node-id,n", po::value<std::string>(), "node id")
            ("node-uuid,w", po::value<fds_uint64_t>(), "node uuid")
            ("volume-id,i", po::value<int>(), "volume id")
            ("domain-id,k", po::value<int>(), "domain id")
            ("throttle-level,t", po::value<float>(), "throttle level")
            ("iops-min,g", po::value<double>(), "minimum IOPS")
            ("iops-max,m", po::value<double>(), "maximum IOPS")
            ("rel-prio,r", po::value<int>(), "relative priority")
            ("enable-service,e", po::value<std::string>()->default_value("sm,dm,am"),
             "service to enable (if dm and sm, speficy \"dm,sm\")")
            ("vol-type,y", po::value<std::string>()->default_value("s3"),
             "volume access type")
            ("media-policy,M", po::value<std::string>()->default_value("hdd"),
             "media policy (hdd, ssd, hybrid, hybrid-prefcap")
            ("om_ip", po::value<std::string>(),
             "OM IP addr") /* Consumed already */
            ("om_port", po::value<fds_uint32_t>(),
             "OM config port"), /* Consumed already */
            ("volume-snap", po::value<std::string>(),
             "Create Snap: volume-snap <vol name>");

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
        LOGNOTIFY << "Constructing the CLI";
        LOGNOTIFY << " Create Volume ";
        LOGNOTIFY << vm["volume-create"].as<std::string>() << " -volume name";
        LOGNOTIFY << vm["volume-size"].as<double>() << " -volume size";
        LOGNOTIFY << vm["volume-policy"].as<int>() << " -volume policy";
        LOGNOTIFY << vm["volume-id"].as<int>() << " -volume id";

        FDS_ProtocolInterface::FDSP_CreateVolType volData;
        volData.vol_name = vm["volume-create"].as<std::string>();
        volData.vol_info.vol_name = vm["volume-create"].as<std::string>();
        volData.vol_info.tennantId = 0;
        volData.vol_info.localDomainId = 0;

        volData.vol_info.capacity = vm["volume-size"].as<double>();
        volData.vol_info.volType =
                stringToVolType(vm.count("vol-type") ?
                                vm["vol-type"].as<std::string>() : "");

        if (fpi::FDSP_VOL_S3_TYPE == volData.vol_info.volType) {
            volData.vol_info.maxObjSizeInBytes = 2097152;  // 2 MiB
        } else {
            volData.vol_info.maxObjSizeInBytes = 4096;  // 4k
        }

        volData.vol_info.volPolicyId = vm["volume-policy"].as<int>();
        volData.vol_info.placementPolicy = 0;
        volData.vol_info.mediaPolicy =
                stringToMediaPolicy(vm.count("media-policy") ?
                                vm["media-policy"].as<std::string>() : "");

        NETWORKCHECK(return_code = cfgPrx->CreateVol(msg_hdr, volData));
        if (return_code !=0) {
            auto sret = std::system("clear");
            cout << "Error: Creating the Volume \n";
        }

    } else if (vm.count("volume-modify") && vm.count("volume-size")) {
        LOGNOTIFY << " Modify Volume ";
        LOGNOTIFY << vm["volume-modify"].as<std::string>() << " -volume name";
        LOGNOTIFY << vm["volume-size"].as<double>() << " -volume size";
        if (vm.count("volume-policy")) {
            LOGNOTIFY << vm["volume-policy"].as<int>() << " -volume policy";
            if (vm.count("iops-min") || vm.count("iops-max") || vm.count("rel-prio")) {
                LOGNOTIFY << "Since prolicy id is specified, "
                          << "min/max iops and priority will be ignored";
            }
        } else {
            if ((vm.count("iops-min") == 0) ||
                (vm.count("iops-max") == 0) ||
                (vm.count("rel-prio") == 0)) {
                LOGERROR << "If 'volume-policy' is not specified, "
                         << "must specify min/max iops and relative priority";
                return 0;
            }
            LOGNOTIFY << vm["iops-min"].as<double>() << " -minimum iops";
            LOGNOTIFY << vm["iops-max"].as<double>() << " -maximum iops";
            LOGNOTIFY << vm["rel-prio"].as<int>() << " -relative priority";
        }
        if (vm.count("media-policy")) {
            LOGNOTIFY << vm["media-policy"].as<std::string>() << " -media policy";
        }

        FDS_ProtocolInterface::FDSP_ModifyVolType volData;
        volData.vol_name = vm["volume-modify"].as<std::string>();
        volData.vol_desc.vol_name = volData.vol_name;

        volData.vol_desc.capacity = vm["volume-size"].as<double>();
        volData.vol_desc.volType =
                stringToVolType(vm.count("vol-type") ?
                                vm["vol-type"].as<std::string>() : "");

        if (fpi::FDSP_VOL_S3_TYPE == volData.vol_desc.volType) {
            volData.vol_desc.maxObjSizeInBytes = 2097152;  // 2 MiB
        } else {
            volData.vol_desc.maxObjSizeInBytes = 4096;  // 4k
        }

        volData.vol_desc.placementPolicy = 0;
        volData.vol_desc.mediaPolicy = vm.count("media_policy") ?
                stringToMediaPolicy(vm["media-policy"].as<std::string>()) :
                FDS_ProtocolInterface::FDSP_MEDIA_POLICY_UNSET;  // use current policy

        if (vm.count("volume-policy")) {
            volData.vol_desc.volPolicyId = vm["volume-policy"].as<int>();
        } else {
            volData.vol_desc.volPolicyId = 0; /* 0 means don't modify actual policy id */
            volData.vol_desc.iops_min = vm["iops-min"].as<double>();
            volData.vol_desc.iops_max = vm["iops-max"].as<double>();
            volData.vol_desc.rel_prio = vm["rel-prio"].as<int>();
        }
        NETWORKCHECK(cfgPrx->ModifyVol(msg_hdr, volData));

    } else if (vm.count("volume-delete") && vm.count("volume-id")) {
        LOGNOTIFY << " Delete Volume ";
        LOGNOTIFY << vm["volume-delete"].as<std::string>() << " -volume name";
        LOGNOTIFY << vm["volume-id"].as<int>() << " -volume id";

        FDS_ProtocolInterface::FDSP_DeleteVolType volData;
        volData.vol_name = vm["volume-delete"].as<std::string>();

        NETWORKCHECK(cfgPrx->DeleteVol(msg_hdr, volData));

    } else if (vm.count("volume-get")) {
        LOGNOTIFY << " Get Volume Info";
        LOGNOTIFY << vm["volume-get"].as<std::string>() << " -volume name";

        FDS_ProtocolInterface::FDSP_GetVolInfoReqType vol_req;
        vol_req.vol_name = vm["volume-get"].as<std::string>();
        vol_req.domain_id = 0;

        FDS_ProtocolInterface::FDSP_VolumeDescType vol_info;
        try {
            NETWORKCHECK(cfgPrx->GetVolInfo(vol_info, msg_hdr, vol_req));
            cout << "Volume " << vol_info.vol_name << ":"
                 << std::hex << vol_info.volUUID << std::dec << std::endl
                 << "     capacity " << vol_info.capacity << " MB"
                 << ", iops_min " << vol_info.iops_min
                 << ", iops_max " << vol_info.iops_max
                 << ", priority " << vol_info.rel_prio
                 << ", media policy " << mediaPolicyToString(vol_info.mediaPolicy)
                 << std::endl;
        } catch(...) {
            cout << "Got non-network exception, probably volume not found" << std::endl;
        }

    } else if (vm.count("volume-attach") &&
               vm.count("volume-id") &&
               vm.count("node-id")) {
        LOGNOTIFY << " Attach Volume ";
        LOGNOTIFY << vm["volume-attach"].as<std::string>() << " -volume name";
        LOGNOTIFY << vm["volume-id"].as<int>() << " -volume id";
        LOGNOTIFY << vm["node-id"].as<std::string>() << " -node id";

        FDS_ProtocolInterface::FDSP_AttachVolCmdType volData;
        volData.vol_name = vm["volume-attach"].as<std::string>();
        volData.node_id = vm["node-id"].as<std::string>();
        msg_hdr.src_node_name = vm["node-id"].as<std::string>();

        NETWORKCHECK(cfgPrx->AttachVol(msg_hdr, volData));

    } else if (vm.count("volume-detach") &&
               vm.count("volume-id") &&
               vm.count("node-id")) {
        LOGNOTIFY << " Detach Volume ";
        LOGNOTIFY << vm["volume-detach"].as<std::string>() << " -volume name";
        LOGNOTIFY << vm["volume-id"].as<int>() << " -volume id";
        LOGNOTIFY << vm["node-id"].as<std::string>() << " -node id";

        FDS_ProtocolInterface::FDSP_AttachVolCmdType volData;
        volData.vol_name = vm["volume-detach"].as<std::string>();
        volData.node_id = vm["node-id"].as<std::string>();
        msg_hdr.src_node_name = vm["node-id"].as<std::string>();

        NETWORKCHECK(cfgPrx->DetachVol(msg_hdr, volData));
    } else if (vm.count("list-volumes")) {
        LOGNOTIFY << "List volumes";

        std::vector<FDS_ProtocolInterface::FDSP_VolumeDescType> vec;
        NETWORKCHECK(cfgPrx->ListVolumes(vec, msg_hdr));

        for (fds_uint32_t i = 0; i < vec.size(); ++i) {
            cout << "Volume " << vec[i].vol_name << ":"
                 << std::hex << vec[i].volUUID << std::dec << std::endl
                 << "     capacity " << vec[i].capacity << " MB"
                 << ", iops_min " << vec[i].iops_min
                 << ", iops_max " << vec[i].iops_max
                 << ", priority " << vec[i].rel_prio
                 << ", media policy " << mediaPolicyToString(vec[i].mediaPolicy)
                 << std::endl;
        }
    } else if ( vm.count("policy-create") && vm.count("volume-policy") && \
                vm.count("iops-max") && vm.count("iops-max") && vm.count("rel-prio")) {
        LOGNOTIFY << " Create Policy ";
        LOGNOTIFY << vm["policy-create"].as<std::string>() << " -policy name";
        LOGNOTIFY << vm["volume-policy"].as<int>() << " -policy id";
        LOGNOTIFY << vm["iops-min"].as<double>() << " -minimum iops";
        LOGNOTIFY << vm["iops-max"].as<double>() << " -maximum iops";
        LOGNOTIFY << vm["rel-prio"].as<int>() << " -relative priority";

        FDS_ProtocolInterface::FDSP_CreatePolicyType policyData;
        policyData.policy_name = vm["policy-create"].as<std::string>();

        policyData.policy_info.policy_name = vm["policy-create"].as<std::string>();
        policyData.policy_info.policy_id = vm["volume-policy"].as<int>();
        policyData.policy_info.iops_min = vm["iops-min"].as<double>();
        policyData.policy_info.iops_max = vm["iops-max"].as<double>();
        policyData.policy_info.rel_prio = vm["rel-prio"].as<int>();

        NETWORKCHECK(cfgPrx->CreatePolicy(msg_hdr, policyData));

    } else if (vm.count("policy-delete") && vm.count("volume-policy")) {
        LOGNOTIFY << " Delete Policy ";
        LOGNOTIFY << vm["policy-delete"].as<std::string>() << " -policy name";
        LOGNOTIFY << vm["volume-policy"].as<int>() << " -policy id";

        FDS_ProtocolInterface::FDSP_DeletePolicyType policyData;

        policyData.policy_name = vm["policy-delete"].as<std::string>();
        policyData.policy_id = vm["volume-policy"].as<int>();

        NETWORKCHECK(cfgPrx->DeletePolicy(msg_hdr, policyData));

    } else if (vm.count("policy-modify") && vm.count("volume-policy") && \
               vm.count("iops-max") && vm.count("iops-max") && vm.count("rel-prio")) {
        LOGNOTIFY << " Modify Policy ";
        LOGNOTIFY << vm["policy-modify"].as<std::string>() << " -policy name";
        LOGNOTIFY << vm["volume-policy"].as<int>() << " -policy id";
        LOGNOTIFY << vm["iops-min"].as<double>() << " -minimum iops";
        LOGNOTIFY << vm["iops-max"].as<double>() << " -maximum iops";
        LOGNOTIFY << vm["rel-prio"].as<int>() << " -relative priority";

        FDS_ProtocolInterface::FDSP_ModifyPolicyType policyData;
        policyData.policy_name = vm["policy-modify"].as<std::string>();

        policyData.policy_info.policy_name = vm["policy-modify"].as<std::string>();
        policyData.policy_info.policy_id = vm["volume-policy"].as<int>();
        policyData.policy_info.iops_min = vm["iops-min"].as<double>();
        policyData.policy_info.iops_max = vm["iops-max"].as<double>();
        policyData.policy_info.rel_prio = vm["rel-prio"].as<int>();

        NETWORKCHECK(cfgPrx->ModifyPolicy(msg_hdr, policyData));

    } else if (vm.count("domain-create") && vm.count("domain-id")) {
        LOGNOTIFY << " Domain Create ";
        LOGNOTIFY << vm["domain-create"].as<std::string>() << "-domain name";
        LOGNOTIFY << vm["domain-id"].as<int>() <<  " -domain id ";

        FDS_ProtocolInterface::FDSP_CreateDomainType domainData;
        domainData.domain_name = vm["domain-create"].as<std::string>();
        domainData.domain_id = vm["domain-id"].as<int>();

        NETWORKCHECK(cfgPrx->CreateDomain(msg_hdr, domainData));

    }  else if (vm.count("list-services")) {

        LOGNOTIFY << " List services ";

        std::vector<FDS_ProtocolInterface::FDSP_Node_Info_Type> vec;
        NETWORKCHECK(cfgPrx->ListServices(vec, msg_hdr));

        for (fds_uint32_t i = 0; i < vec.size(); ++i) {

            cout << "Node UUID " << std::hex << vec[i].node_uuid << std::dec
                 << std::endl
                 << "\tState " << vec[i].node_state << std::endl
                 << "\tType " << vec[i].node_type << std::endl
                 << "\tName " << vec[i].node_name << std::endl
                 << "\tUUID " << std::hex << vec[i].service_uuid << std::dec
                 << std::endl
                 << "\tIPv6 " << netSession::ipAddr2String(vec[i].ip_hi_addr)
                 << std::endl
                 << "\tIPv4 " << netSession::ipAddr2String(vec[i].ip_lo_addr)
                 << std::endl
                 << "\tControl Port " << vec[i].control_port << std::endl
                 << "\tData Port " << vec[i].data_port << std::endl
                 << "\tMigration Port " << vec[i].migration_port << std::endl
                 << "\tMetasync Port " << vec[i].metasync_port << std::endl
                 << "\tNode Root " << vec[i].node_root << std::endl
                 << std::endl;
        }

    }  else if (vm.count("remove-services")) {
        LOGNOTIFY << " Remove services ";
        LOGNOTIFY << vm["remove-services"].as<std::string>() << "- node name";
        FDS_ProtocolInterface::FDSP_RemoveServicesType removeServiceData;
        removeServiceData.node_name = vm["remove-services"].as<std::string>();
        removeServiceData.node_uuid.uuid = 0;
        removeServiceData.remove_sm = false;
        removeServiceData.remove_dm = false;
        removeServiceData.remove_am = false;
        // even though it is called enable-service, we just re-using same
        // command, and here it means remove service
        std::string svc_str = vm["enable-service"].as<std::string>();
        boost::char_separator<char> sep(",");
        boost::tokenizer<boost::char_separator<char>> tokens(svc_str, sep);
        for (const auto& t : tokens) {
            if (t.compare("am") == 0) {
                removeServiceData.remove_am = true;
                LOGNORMAL << "   -- remove AM";
            } else if (t.compare("sm") == 0) {
                removeServiceData.remove_sm = true;
                LOGNORMAL << "   -- remove SM";
            } else if (t.compare("dm") == 0) {
                removeServiceData.remove_dm = true;
                LOGNORMAL << "   -- remove DM";
            } else {
                cout << "Unknown service " << t << ", will ignore" << std::endl;
                LOGWARN << "Unknown service " << t << "is specified; "
                        << "will ignore, but check your fdscli command";
            }
        }

        NETWORKCHECK(cfgPrx->RemoveServices(msg_hdr, removeServiceData));
    }  else if (vm.count("activate-nodes")) {
        LOGNOTIFY << " Activate Nodes: domain name "
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
                LOGNORMAL << "   -- activate AM";
            } else if (t.compare("sm") == 0) {
                activateNodesData.activate_sm = true;
                LOGNORMAL << "   -- activate SM";
            } else if (t.compare("dm") == 0) {
                activateNodesData.activate_dm = true;
                LOGNORMAL << "   -- activate DM";
            } else {
                cout << "Unknown service " << t << ", will ignore" << std::endl;
                LOGWARN << "Unknown service " << t << "is specified; "
                        << "will ignore, but check your fdscli command";
            }
        }

        NETWORKCHECK(cfgPrx->ActivateAllNodes(msg_hdr, activateNodesData));
    }  else if (vm.count("activate-services")) {
        LOGNOTIFY << " Activate Services: domain name "
                  << vm["activate-services"].as<std::string>()
                  << " (domain name ignored for now, using default domain)"
                  << " services: " << vm["enable-service"].as<std::string>()
                  << " on node " << std::hex << vm["node-uuid"].as<fds_uint64_t>()
                  << std::dec;

        FDS_ProtocolInterface::FDSP_ActivateOneNodeType activateNodeData;
        activateNodeData.domain_id = vm["domain-id"].as<int>();
        activateNodeData.node_uuid.uuid = vm["node-uuid"].as<fds_uint64_t>();
        activateNodeData.activate_sm = false;
        activateNodeData.activate_dm = false;
        activateNodeData.activate_am = false;
        std::string svc_str = vm["enable-service"].as<std::string>();
        boost::char_separator<char> sep(",");
        boost::tokenizer<boost::char_separator<char>> tokens(svc_str, sep);
        for (const auto& t : tokens) {
            if (t.compare("am") == 0) {
                activateNodeData.activate_am = true;
                LOGNORMAL << "   -- activate AM";
            } else if (t.compare("sm") == 0) {
                activateNodeData.activate_sm = true;
                LOGNORMAL << "   -- activate SM";
            } else if (t.compare("dm") == 0) {
                activateNodeData.activate_dm = true;
                LOGNORMAL << "   -- activate DM";
            } else {
                cout << "Unknown service " << t << ", will ignore" << std::endl;
                LOGWARN << "Unknown service " << t << "is specified; "
                        << "will ignore, but check your fdscli command";
            }
        }

        NETWORKCHECK(return_code = cfgPrx->ActivateNode(msg_hdr, activateNodeData));
        if (return_code != 0) {
            Error err(return_code);
            cout << "Failed to activate services, result "
                 << err.GetErrstr() << std::endl;
        }
    }  else if (vm.count("domain-delete") && vm.count("domain-id")) {
        LOGNOTIFY << " Domain Delete ";
        LOGNOTIFY << vm["domain-delete"].as<std::string>() << "-domain name";
        LOGNOTIFY << vm["domain-id"].as<int>() <<  " -domain id ";

        FDS_ProtocolInterface::FDSP_CreateDomainType domainData;
        domainData.domain_name = vm["domain-delete"].as<std::string>();
        domainData.domain_id = vm["domain-id"].as<int>();

        NETWORKCHECK(cfgPrx->DeleteDomain(msg_hdr, domainData));
    }  else if (vm.count("domain-shutdown") && vm.count("domain-id")) {
        LOGNOTIFY << " Domain Shutdown ";
        LOGNOTIFY << vm["domain-shutdown"].as<std::string>() << "-domain name";
        LOGNOTIFY << vm["domain-id"].as<int>() <<  " -domain id ";

        FDS_ProtocolInterface::FDSP_ShutdownDomainType domainData;
        domainData.domain_id = vm["domain-id"].as<int>();

        NETWORKCHECK(cfgPrx->ShutdownDomain(msg_hdr, domainData));
    } else if (vm.count("domain-stats") && vm.count("domain-id")) {
        LOGNOTIFY << " Domain Stats ";
        LOGNOTIFY << vm["domain-id"].as<int>() <<  " -domain id ";

        FDS_ProtocolInterface::FDSP_GetDomainStatsType domainData;
        domainData.domain_id = vm["domain-id"].as<int>();

        NETWORKCHECK(cfgPrx->GetDomainStats(msg_hdr, domainData));

    } else if (vm.count("throttle") && vm.count("throttle-level")) {
        LOGNOTIFY << " Throttle ";
        LOGNOTIFY << vm["throttle-level"].as<float>() << "-throttle_level";

        FDS_ProtocolInterface::FDSP_ThrottleMsgType throttle_msg;
        throttle_msg.domain_id = 0;
        throttle_msg.throttle_level = vm["throttle-level"].as<float>();

        NETWORKCHECK(cfgPrx->SetThrottleLevel(msg_hdr, throttle_msg));
    } else if (vm.count("scavenger")) {
        std::string cmd = vm["scavenger"].as<string>();
        if ((cmd.compare("start") == 0) || (cmd.compare("stop") == 0) ||
            (cmd.compare("enable") == 0) || (cmd.compare("disable") == 0)) {
            LOGNOTIFY << "Will do Scavenger " << cmd << " command";

            FDS_ProtocolInterface::FDSP_ScavengerType gc_msg;
            gc_msg.cmd = stringToScavengerCommand(cmd);
            NETWORKCHECK(cfgPrx->ScavengerCommand(msg_hdr, gc_msg));
        } else {
            LOGNOTIFY << "Unrecognized scavenger command!";
            std::cout << "Unrecognized scavenger command";
        }
    } else if (vm.count("volume-snap")) {
        LOGNOTIFY << "Constructing the CLI";
        LOGNOTIFY << " Snap Volume ";
        LOGNOTIFY << vm["volume-snap"].as<std::string>() << " -volume name";

        FDS_ProtocolInterface::FDSP_CreateVolType volData;
        volData.vol_name = vm["volume-snap"].as<std::string>();
        volData.vol_info.vol_name = vm["volume-snap"].as<std::string>();

        NETWORKCHECK(return_code = cfgPrx->SnapVol(msg_hdr, volData));
        if (return_code !=0) {
            auto sret = std::system("clear");
            cout << "Error: Snap Volume \n";
        }
    } else {
        gl_OMCli.setCliClient(cfgPrx);
        gl_OMCli.mod_run();
    }

    endClientSession();
    return 0;
}

int FdsCli::run()
{
    int    argc;
    char **argv;

    argv = mod_vectors_->mod_argv(&argc);

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

    return fdsCli->fdsCliParser(argc, argv);
}

}  // namespace fds

int main(int argc, char* argv[])
{
    fds::Module *cliVec[] = {
        &fds::gl_OMCli,
        nullptr
    };
    fds::fdsCli = new fds::FdsCli(argc, argv,
                                  "platform.conf", "fds.om.", "cli.log", cliVec);
    int retcode = fds::fdsCli->main();
    delete fds::fdsCli;
    return retcode;
}

