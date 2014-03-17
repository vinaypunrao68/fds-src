#include "configdbtool.h"

#define LINE cout << "  " 
#define ERRORLINE LINE << Color::Red    << "[error] : " << Color::End 
#define WARNLINE  LINE << Color::Yellow << "[warn ] : " << Color::End 
#define REGISTERCMD(c,fn) registerCommand(c, (CmdCallBack)&ConfigDBTool::cmd##fn);

ConfigDBTool::ConfigDBTool(std::string host, int port) {
    db = new kvstore::ConfigDB(host,port,1);

    if (!db->isConnected()) {
        ERRORLINE << "unable to connect to db @ [" << host << ":" << port << "]\n";
        exit(1);
    }
    
    setPrompt("fds:configdb:> ");
    REGISTERCMD("info",Info);
    REGISTERCMD("listvolumes",ListVolumes);
    REGISTERCMD("volume",Volume);
    REGISTERCMD("listpolicies",ListPolicies);
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

    LINE << "global.domain : " << db->getGlobalDomain() << "\n";

    std::map<int,std::string> mapDomains;
    db->getLocalDomains(mapDomains);
    LINE << "localdomains  : " << mapDomains.size() << "\n";
    
    std::vector<VolumeDesc> volumes;
    db->getVolumes(volumes);
    LINE << "no.of volumes : " << volumes.size() << "\n";

    std::vector<FDS_VolumePolicy> policies;
    db->getPolicies(policies);
    LINE << "no.of policies : " << policies.size() << "\n";
    
}

void ConfigDBTool::cmdListVolumes(std::vector <std::string>& args) {
    if (!check()) return;

    std::vector<VolumeDesc> volumes;
    db->getVolumes(volumes);
    LINE << Color::BoldWhite << "Num volumes : " << Color::End << volumes.size() << "\n";

    for (uint i=0 ; i < volumes.size() ; i++) {
        LINE << (i+1) << " : " << volumes[i].volUUID << " : " << volumes[i].name << "\n";
    }
}

void ConfigDBTool::cmdListPolicies(std::vector <std::string>& args) {
    if (!check()) return;

    std::vector<FDS_VolumePolicy> policies;
    db->getPolicies(policies);

    LINE << Color::BoldWhite << "Num Policies : " << Color::End << policies.size() << "\n";

    for (uint i=0 ; i < policies.size() ; i++) {
        LINE << (i+1) << " : " << policies[i] << "\n";
    }
}

void ConfigDBTool::cmdVolume(std::vector <std::string>& args) {
    if (!check()) return;
    if (args.empty()) {
        ERRORLINE << "need atleast one vol uuid" << "\n";
        return;                
    }

    for (uint i = 0 ; i < args.size() ; i++ ) {
        fds_volid_t volumeId = strtoull(args[i].c_str(),NULL,10);
        VolumeDesc volumeDesc("",1);
        if (!db->getVolume(volumeId,volumeDesc)) {
            WARNLINE << "could not find volume info for uuid : " << volumeId << "\n";
        } else {
            LINE << Color::BoldWhite << volumeId << Color::End << " : " << volumeDesc ; 
            cout << "\n";
        }    
    }
}

void usage() {
    cout <<"usage : configdb [-h host] [-p port] " << endl;
    exit(1);
}

int main(int argc, char* argv[]) {

    int i = 0;
    std::string host = "localhost";
    int port = 6379;

    bool fLast = ( i == argc-1 );
    for (i = 1 ; i < argc ; i++ ) {        
        if (!strcmp(argv[i],"-h")) {
            if (fLast) usage();
            host = argv[++i];
        } else if (!strcmp(argv[i],"-p")) {
            if (fLast) usage();
            port = atoi(argv[++i]);
        } else if (!strcmp(argv[i],"--help")) {
            usage();
        } else {
            ERRORLINE << "unknonwn arg : " << argv[i];
        }
    }

    ConfigDBTool tool(host,port);
    tool.run();
}
