/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

#include <algorithm>
#include <boost/program_options.hpp>
#include <dmchk.h>
#include <vector>
namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    fds_volid_t volumeUuid = invalid_vol_id;
    std::string blobName;
    // process command line options
    po::options_description desc("\nDM checker command line options");
    po::positional_options_description m_positional;
    desc.add_options()
            ("help,h", "help/ usage message")
            ("list,l", "list all volumes")
            ("blobs,b", "list blob contents for given volume")
            ("stats,s", "show stats")
            ("objects,o", po::value<std::string>(&blobName), "show objects for blob")
            ("volumeid", po::value<std::vector<uint64_t> >()->composing());
    m_positional.add("volumeid", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
              .options(desc)
              .allow_unregistered()
              .positional(m_positional)
              .run(), vm);
    po::notify(vm);

    if (vm.count("help") || !(vm.count("list") || vm.count("blobs") || vm.count("stats") || vm.count("objects"))) {
        std::cout << desc << std::endl;
        return 0;
    }

    std::vector<fds_volid_t> volumes, allVolumes;

    // Transform the provided uint64s into fds_volid_t's
    if (!vm["volumeid"].empty()) {
        auto parsedIds = vm["volumeid"].as<std::vector<uint64_t> >();
        std::transform(parsedIds.begin(), parsedIds.end(), std::back_inserter(volumes),
           [](uint64_t const& val) -> fds_volid_t { return fds_volid_t(val); });
    }

    Module *dmchkVec[] = {
        nullptr
    };
    
    DmChecker dmchk(argc, argv, "platform.conf", "fds.dm.", dmchkVec, "DM checker driver");

    if (volumes.empty() || vm.count("list")) {
        dmchk.getVolumeIds(allVolumes);
    }

    if (volumes.empty()) {
        volumes = allVolumes;
    }

    if (vm.count("list")) {
        int count = 0;
        std::cout << "volumes in the system >> " << std::endl;
        std::cout << "no  :  volid " << std::endl;
        for (const auto v : allVolumes) {
            std::cout << "[" << ++count << "] : volid:" << v << std::endl;
        }
        std::cout << std::endl;
    }
    
    if (vm.count("blobs") || vm.count("stats")) {
        bool fStatOnly = !vm.count("blobs");
        for (const auto v : volumes) {
            if (dmchk.loadVolume(v).ok()) {
                dmchk.listBlobs(fStatOnly);
            } else {
                std::cout << "unable to load volume:" << v << std::endl;
            }
        }
    }

    if (vm.count("objects")) {
        for (const auto v : volumes) {
            if (dmchk.loadVolume(v).ok()) {
                dmchk.blobInfo(blobName);
            } else {
                std::cout << "unable to load volume:" << v << std::endl;
            }
        }
    }
    return 0;
}
