#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <utility>
#include <map>

#include "configdbtool.h"
#include "platform/node_data.h"

#define LINE std::cout << "  "
#define ERRORLINE LINE << Color::Red    << "[error] : " << Color::End
#define WARNLINE  LINE << Color::Yellow << "[warn ] : " << Color::End
#define REGISTERCMD(c, fn) registerCommand(c, (CmdCallBack)&ConfigDBTool::cmd##fn);

using PAIR = std::pair<std::string, std::string>;

#define PRINTSTREAM(x) {                        \
        std::ostringstream oss;                 \
        oss << x;                               \
        printData(oss.str()); }

namespace fds {

void printData(std::string data) {
    size_t pos;
    pos = data.find('['); if (pos != std::string::npos) data[pos]= ' ';
    pos = data.find(']'); if (pos != std::string::npos) data[pos]= ' ';

    std::istringstream f(data);
    std::string s;
    std::vector<std::pair<std::string, std::string> > nameValues;
    while (std::getline(f, s, ' ')) {
        pos = s.find(':');
        if (pos != std::string::npos) {
            nameValues.push_back(std::pair<std::string, std::string> (s.substr(0, pos), s.substr(pos+1)));
        } else {
            if (!s.empty()) {
                nameValues.push_back(std::pair<std::string, std::string>(s, ""));
            }
        }
    }

    std::string name, value;
    for (uint i = 0; i < nameValues.size(); i++ ) {
        LINE << std::setw(20) << nameValues[i].first <<  " : " << nameValues[i].second << "\n";
    }
}

unsigned long long getUUID(const std::string& encoded) {
    unsigned long long l;
    sscanf(encoded.c_str() , "%llx", &l);
    return l;
}

ConfigDBTool::ConfigDBTool(std::string host, int port) {
    db = new fds::kvstore::ConfigDB(host, port, 1);

    setHistoryFile("/tmp/.confidbtool.hist");
    if (!db->isConnected()) {
        ERRORLINE << "unable to connect to db @ [" << host << ":" << port << "]\n";
        exit(1);
    }

    setPrompt("fds:configdb:> ");
    REGISTERCMD("info", Info);
    REGISTERCMD("listvolumes", ListVolumes);
    REGISTERCMD("volume", Volume);
    REGISTERCMD("listpolicies", ListPolicies);
    REGISTERCMD("listnodes", ListNodes);
    REGISTERCMD("node", Node);
    REGISTERCMD("dlt", Dlt);
    REGISTERCMD("help", Help);

    // help data

    helpInfo.push_back({"help", "[command]", "display help for all or a specific command(s)"});
    helpInfo.push_back({"info", "", "display basic info about the config db"});
    helpInfo.push_back({"dlt", "[next/current]", "show the dlt "});
    helpInfo.push_back({"volume", "[volid ...]", "show volume info for the given ids "});
    helpInfo.push_back({"node", "[nodeid ...]", "show node info for the given ids "});
    helpInfo.push_back({"listvolumes", "", "show the list of volumes"});
    helpInfo.push_back({"listpolicies", "", "show the list of policies "});
    helpInfo.push_back({"listnodes", "", "show the list of nodes"});
}

bool ConfigDBTool::check() {
    if (!db->isConnected()) {
        ERRORLINE  << "unable to talk to configdb" << "\n";
        return false;
    }
    return true;
}

void ConfigDBTool::cmdInfo(std::vector <std::string>& args) {
    if (!check()) return;

    LINE << "global.domain   : " << db->getGlobalDomain() << "\n";

    std::vector<LocalDomain> localDomains;
    db->listLocalDomains(localDomains);
    LINE << "localdomains    : " << localDomains.size() << "\n";

    std::vector<VolumeDesc> volumes;
    db->getVolumes(volumes);
    LINE << "no.of volumes   : " << volumes.size() << "\n";

    std::vector<FDS_VolumePolicy> policies;
    db->getPolicies(policies);
    LINE << "no.of policies  : " << policies.size() << "\n";
}

void ConfigDBTool::cmdListVolumes(std::vector <std::string>& args) {
    if (!check()) return;

    std::vector<VolumeDesc> volumes;
    db->getVolumes(volumes);
    LINE << Color::BoldWhite << "Num volumes : " << Color::End << volumes.size() << "\n";

    for (uint i= 0; i < volumes.size(); i++) {
        LINE << (i+1) << " : " << volumes[i].volUUID << " : " << volumes[i].name << "\n";
    }
}

void ConfigDBTool::cmdListNodes(std::vector <std::string>& args) {
    if (!check()) return;

    std::vector<struct node_data> nodes;
    db->getAllNodes(nodes);
    LINE << Color::BoldWhite << "Num nodes : " << Color::End << nodes.size() << "\n";

    for (uint i= 0; i < nodes.size(); i++) {
        LINE << (i+1) << " : " << nodes[i].nd_node_uuid << " : " << nodes[i].nd_assign_name << "\n";
    }
}


void ConfigDBTool::cmdListPolicies(std::vector <std::string>& args) {
    if (!check()) return;

    std::vector<FDS_VolumePolicy> policies;
    db->getPolicies(policies);

    LINE << Color::BoldWhite << "Num Policies : " << Color::End << policies.size() << "\n";

    for (uint i= 0; i < policies.size(); i++) {
        LINE << (i+1) << " : \n";
        PRINTSTREAM(policies[i]);
        std::cout << "\n";
    }
}

void ConfigDBTool::cmdVolume(std::vector <std::string>& args) {
    if (!check()) return;
    if (args.empty()) {
        ERRORLINE << "need atleast one vol uuid" << "\n";
        return;
    }

    for (uint i = 0; i < args.size(); i++ ) {
        fds_volid_t volumeId = strtoull(args[i].c_str(), NULL, 10);
        VolumeDesc volume("", 1);

        bool fFound = true;
        if (!db->getVolume(volumeId, volume)) {
            volumeId = getUUID(args[i]);
            if (!db->getVolume(volumeId, volume)) {
                fFound = false;
            }
        }

        if (!fFound) {
            WARNLINE << "could not find volume info for uuid : " << args[i] << "\n";
        } else {
            LINE << Color::BoldWhite << volumeId << Color::End << "\n";
            PRINTSTREAM(volume);
            std::cout << "\n";
        }
    }
}

void ConfigDBTool::cmdNode(std::vector <std::string>& args) {
    if (!check()) return;
    if (args.empty()) {
        ERRORLINE << "need atleast one node uuid" << "\n";
        return;
    }

    bool fFound = false;
    for (uint i = 0; i < args.size(); i++) {
        fds_volid_t nodeId = strtoull(args[i].c_str(), NULL, 10);
        node_data node;
        fFound = true;
        if (!db->getNode(nodeId, &node)) {
            nodeId = getUUID(args[i]);
            if (!db->getNode(nodeId, &node)) {
                fFound = false;
            }
        }

        if (!fFound) {
            WARNLINE << "could not find node info for uuid : " << args[i]  << "\n";
        } else {
            LINE << Color::BoldWhite << nodeId << Color::End << "\n";
            LINE << node.nd_node_uuid << " : " << node.nd_assign_name << "\n";
            // PRINTSTREAM(node);
            NodeServices services;
            if (db->getNodeServices(nodeId, services)) {
                LINE << Color::BoldWhite << "Services::" << Color::End << services << "\n";
            }
            std::cout << "\n";
        }
    }
}

void ConfigDBTool::cmdDlt(std::vector <std::string>& args) {
    if (!check()) return;
    std::string type = "current";
    if (!args.empty()) {
        if (args[0] != "next" || args[0] != "current" ) {
            ERRORLINE << "invalid arg : " << args[0] << "\n";
            return;
        }
        type = args[0];
    }

    fds_uint64_t version = db->getDltVersionForType(type);
    if (version > 0 ) {
        DLT dlt(0, 0, 0, false);
        if (!db->getDlt(dlt, version)) {
            WARNLINE << "unable to get  dlt version [" << version << "]" <<"\n";
            return;
        }
        dlt.generateNodeTokenMap();
        LINE << dlt << "\n";
    }
}

void ConfigDBTool::cmdHelp(std::vector <std::string>& args) {
    for (uint i = 0; i < helpInfo.size(); i++) {
        LINE << "\n";
        LINE << Color::Yellow << helpInfo[i].command << Color::End << " : " << helpInfo[i].args << "\n";
        LINE << "desc: " << helpInfo[i].description << "\n";
    }
    LINE "\n";
}

}  // namespace fds

void usage() {
    std::cout <<"usage : configdb [-h host] [-p port] " << std::endl;
    exit(1);
}


int main(int argc, char* argv[]) {
    int i = 0;
    std::string host = "localhost";
    int port = 6379;

    bool fLast = (i == argc-1);
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h")) {
            if (fLast) usage();
            host = argv[++i];
        } else if (!strcmp(argv[i], "-p")) {
            if (fLast) usage();
            port = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--help")) {
            usage();
        } else {
            ERRORLINE << "unknonwn arg : " << argv[i];
        }
    }

    fds::ConfigDBTool tool(host, port);
    tool.run();
}
