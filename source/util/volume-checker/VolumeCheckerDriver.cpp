/*
 * Copyright 2016 Formation Data Systems, Inc.
 * A simple, C-Styled argument parser and usage helper to help
 * drive the main VolumeChecker process.
 */

#include <VolumeChecker.h>
#include <unistd.h>
#include <TestUtils.h>
#include <string.h>
#include <stdio.h>

/* Globals to drive Volume Checker */
bool help_flag;
int pm_uuid = -1;
int pm_port = -1;
int min_argc = 0;
std::string volumeList;

/**
 * Helper Functions
 */

void displayUsage(std::string &progName) {
    std::cout << "Usage: " << progName << " [-h] -u <PM UUID> -p <PM Port> -v <List of volumes (CSV)>" << std::endl;
    std::cout << "" << std::endl;
}

void displayHelp(std::string &progName) {
    displayUsage(progName);
    std::cout << "Brings a list of volumes offline for consistency check." << std::endl;
    std::cout << "This program must be run from the OM node, and the OM must be up." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "List of arguments: " << std::endl;
    std::cout << " -h : Displays this help page." << std::endl;
    std::cout << " -u : Current node's PM's UUID in decimal." << std::endl;
    std::cout << " -p : Current node's PM's port in decimal." << std::endl;
    std::cout << " -v : A list of volumes to be checked separated by commas." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << progName << " -u 2059 -p 9861 -v 2,3,5" << std::endl;
    std::cout << "" << std::endl;
}

void parseArgs(int argc, char **argv) {
    int opt, flags;
    bool pmPortSet = false;
    bool pmUuidSet = false;
    bool volListSet = false;
    std::string progName(argv[0]);

    // When adding a new option below, make sure to increment min_argc
    while ((opt = getopt(argc, argv, "hv:u:p:")) != -1) {
        switch (opt) {
        case 'h':
            help_flag = true;
            break;
        case 'v':
            // TODO - check input
            volumeList = optarg;
            volListSet = true;
            min_argc++;
            break;
        case 'u':
            pm_uuid = atoi(optarg);
            pmUuidSet = true;
            min_argc++;
            break;
        case 'p':
            pm_port = atoi(optarg);
            pmPortSet = true;
            min_argc++;
            break;
        default: /* '?' */
            displayUsage(progName);
            exit(EXIT_FAILURE);
        }
    }

    if (!pmPortSet || !pmUuidSet || !volListSet) {
        displayUsage(progName);
        exit(EXIT_FAILURE);
    }
}

/**
 * Args to be sent are as follows:
 *         {"vc", <-- not part of min_argc
 *          roots[0], <-- not part of min_argc
 *          "--fds.pm.platform_uuid=<uuid>",
 *          "--fds.pm.platform_port=<port>",
 *          "-v=1,2,3
 *          }
 *
 *  min_argc == 5 above + 1 for original prog name
 *
 */
void populateInternalArgs(int argc, char **argv, int *internal_argc, char ***internal_argv, std::string root) {

    fds_verify(internal_argc && internal_argv);


    char **new_argv = *internal_argv;
    int new_argc = *internal_argc;

    // see above for increment
    min_argc += 3;

    *internal_argc = min_argc;
    // one for NULL ptr
    *internal_argv = (char **)malloc(sizeof(char*) * (*internal_argc) + 1);
    (*internal_argv)[*internal_argc] = NULL;
    std::string currentArg;

    for (int i = 0; i < *internal_argc; ++i) {
        if (i == 0) {
            currentArg.assign(argv[0]);
        } else if (i == 1) {
            currentArg = "vc";
        } else if (i == 2) {
            currentArg = root;
        } else if (i == 3) {
            currentArg = "--fds.pm.platform_uuid=" + std::to_string(pm_uuid);
        } else if (i == 4) {
            currentArg = "--fds.pm.platform_port=" + std::to_string(pm_port);
        } else if (i == 5) {
            currentArg = "-v=" + volumeList;
        } else {
            break;
        }
        (*internal_argv)[i] = (char *)malloc(sizeof(char) * currentArg.length() + 1);
        strcpy(((*internal_argv)[i]), currentArg.c_str());
    }
}

int main(int argc, char **argv) {

    char **internal_argv = NULL;
    int internal_argc = 0;

    help_flag = (argc > 1) ? false : true;
    parseArgs(argc, argv);
    if (help_flag) {
        std::string progName(argv[0]);
        displayHelp(progName);
        return(0);
    }

    // Set up internal args to pass to checker
    std::vector<std::string> roots;
    fds::TestUtils::populateRootDirectories(roots, 1);

    populateInternalArgs(argc, argv, &internal_argc, &internal_argv, roots[0]);

    fds::VolumeChecker checker(internal_argc, internal_argv, false);
    checker.run();
    return checker.main();
}
