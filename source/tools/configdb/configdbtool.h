/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_TOOLS_CONFIGDB_CONFIGDBTOOL_H_
#define SOURCE_TOOLS_CONFIGDB_CONFIGDBTOOL_H_

#include <cmdconsole.h>
#include <kvstore/configdb.h>
#define COMMAND(Name) void cmd##Name(std::vector <std::string>& args)

struct HelpInfo {
    std::string command;
    std::string args;
    std::string description;
};

class ConfigDBTool : public CmdConsole {
public:
    ConfigDBTool(std::string host = "localhost",int port = 6379);

    bool check();

    COMMAND(Info);
    COMMAND(ListVolumes);
    COMMAND(Volume);
    COMMAND(ListPolicies);
    COMMAND(Dlt);
    COMMAND(Help);

    kvstore::ConfigDB* db;

    std::string host;

    std::vector<HelpInfo> helpInfo;
    int port;
};

#endif
