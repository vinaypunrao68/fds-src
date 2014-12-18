/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <stdio.h>
#include <string>
#include <ep-map.h>
#include <net/net-service.h>
#include <platform/fds_shmem.h>
#include <platform/node-inv-shmem.h>
#include "boost/program_options.hpp"
#include "./shm_dump.h"

namespace fds {

void
ShmDump::read_inv_shm(const char *fname)
{
    FdsShmem *shm_ctrl;
    char *shm;

    shm_ctrl = new FdsShmem(fname, 2097152);
    shm = reinterpret_cast<char *>(shm_ctrl->shm_attach(PROT_READ));

    node_shm_inventory_t *inv = reinterpret_cast<node_shm_inventory_t *>(shm);

    // Header
    std::cout << "Magic: " << inv->shm_magic << std::endl
              << "Version: " << inv->shm_version << std::endl
              << "State: " << std::hex << inv->shm_state
              << std::endl << std::endl;

    // Node inventory
    std::cout << "Node Inventory Offset: " << std::dec << inv->shm_node_inv_off << std::endl
              << "Node Inventory Key Offset: " << inv->shm_node_inv_key_off << std::endl
              << "Node Inventory Key Size: " << inv->shm_node_inv_key_siz << std::endl
              << "Node Inventory Object Size: " << inv->shm_node_inv_obj_siz
              << std::endl << std::endl;

    // UUID inventory
    std::cout << "UUID Bind Offset: " << inv->shm_uuid_bind_off << std::endl
              << "UUID Bind Key Offset: " << inv->shm_uuid_bind_key_off << std::endl
              << "UUID Bind Key Size: " << inv->shm_uuid_bind_key_siz << std::endl
              << "UUID Bind Object Size: " << inv->shm_uuid_bind_obj_siz
              << std::endl << std::endl;


    // AM node inventory data
    std::cout << "AM Inventory Offset: " << inv->shm_am_inv_off << std::endl
              << "AM Inventory Key Offset: " << inv->shm_am_inv_key_off << std::endl
              << "AM Inventory Key Size: " << inv->shm_am_inv_key_siz << std::endl
              << "AM Inventory Object Size: " << inv->shm_am_inv_obj_siz
              << std::endl << std::endl;

    // Print node inventory
    int empty_nodes = 0;
    for (char *node = shm + inv->shm_node_inv_off;
         node < (shm + inv->shm_uuid_bind_off);
         node += inv->shm_node_inv_obj_siz) {
        // ---
        node_data_t *np = reinterpret_cast<node_data_t *>(node);

        int tmp_buf[sizeof(node_data_t)];
        memset(tmp_buf, 0, sizeof(node_data_t));

        if (memcmp(tmp_buf, np, sizeof(node_data_t)) == 0) {
            empty_nodes++;
            continue;
        }

        if (empty_nodes) {
            std::cout << "Empty Inventory Node... x" << empty_nodes << std::endl << std::endl;
            empty_nodes = 0;
        }
        std::cout << "Node UUID: " << std::hex << np->nd_node_uuid << std::endl
                  << "Service Node UUID: " << np->nd_service_uuid << std::endl
                  << "Base port: " <<  std::dec << np->nd_base_port << std::endl
                  << "IP: " << np->nd_ip_addr << std::endl
                  << "Auto Name: " << np->nd_auto_name << std::endl
                  << "Assign Name: " << np->nd_assign_name << std::endl
                  << "SVC Type: " << np->nd_svc_type << std::endl
                  << "SVC Mask: " << np->nd_svc_mask << std::endl
                  << "DLT Version: " << np->nd_dlt_version << std::endl
                  << "Disk Type: " << np->nd_disk_type << std::endl
                  << "Capability: " << std::endl
                  << "\t Capacity: " << np->nd_capability.disk_capacity << std::endl
                  << "\t IOPs Max: " << np->nd_capability.disk_iops_max << std::endl
                  << "\t IOPs Min: " << np->nd_capability.disk_iops_min << std::endl
                  << "\t Disk Latency Max: " << np->nd_capability.disk_latency_max << std::endl
                  << "\t Disk Latency Min: " << np->nd_capability.disk_latency_min << std::endl
                  << "\t SSD IOPs Max: " << np->nd_capability.ssd_iops_max << std::endl
                  << "\t SSD IOPs Min: " << np->nd_capability.ssd_iops_min << std::endl
                  << "\t SSD Latency Max: " << np->nd_capability.ssd_latency_max << std::endl
                  << "\t SSD Latency Min: " << np->nd_capability.ssd_latency_min << std::endl
                  << std::endl << std::endl;
    }
    if (empty_nodes) {
        std::cout << "Empty Inventory Node... x" << empty_nodes << std::endl << std::endl;
        empty_nodes = 0;
    }
    char *uuid = shm + inv->shm_uuid_bind_off;
    for (int i = 0; i < 10240; i++) {
        char          ip[INET6_ADDRSTRLEN];
        int           port;
        ep_map_rec_t *map = reinterpret_cast<ep_map_rec_t *>(uuid);

        port = EpAttr::netaddr_get_port(&map->rmp_addr);
        EpAttr::netaddr_to_str(&map->rmp_addr, ip, sizeof(ip));
        if (map->rmp_uuid != 0) {
            std::cout << "Map uuid " << std::hex << map->rmp_uuid << std::dec
                << " (" << map->rmp_major << ":" << map->rmp_minor
                << " ip " << ip << ":" << port << std::endl;
        }
        uuid += sizeof(*map);
    }
}

void
ShmDump::read_queue_shm(const char *fname)
{
    FdsShmem *shm_ctrl;
    char *shm;

    shm_ctrl = new FdsShmem(fname, 131072);  // Queue is 128k
    shm = reinterpret_cast<char *>(shm_ctrl->shm_attach(PROT_READ));

    int empty_nodes = 0;

    node_shm_queue_t *queue = reinterpret_cast<node_shm_queue_t *>(shm);

    std::cout << "Queue Header Size: " << queue->smq_hdr_size << std::endl
              << "Queue Item Size: " << queue->smq_item_size << std::endl
              << "Queue Sync Structure: " << std::endl
              << "\t Mutex: ... " << std::endl
              << "\t Consumer Condition Variable: ..." << std::endl
              << "\t Producer Condition Variable: ..." << std::endl
              << "N Producer, 1 Consumer Control: " << std::endl
              << "\t Producer Index: " << queue->smq_svc2plat.shm_nprd_idx << std::endl
              << "\t Consumer Index: " << queue->smq_svc2plat.shm_1con_idx << std::endl
              << "1 Producer, N Consumer Control: " << std::endl
              << "\t Producer Index: " << queue->smq_plat2svc.shm_1prd_idx << std::endl
              << "\t Consumer count: " << queue->smq_plat2svc.shm_ncon_cnt << std::endl
              << "\t Consumer Indexes: " << std::endl;
    for (int i = 0; i < queue->smq_plat2svc.shm_ncon_cnt; ++i) {
        std::cout << "\t\t Consumer #" << i << ": "
                  << queue->smq_plat2svc.shm_ncon_idx[i] << std::endl;
    }

    std::cout << std::endl;
    // Queue data
    for (char *node = shm + NodeShmCtrl::shm_queue_hdr;
         node < (shm + (NodeShmCtrl::shm_queue_hdr +
                        (NodeShmCtrl::shm_q_item_size * NodeShmCtrl::shm_q_item_count)));
         node += NodeShmCtrl::shm_q_item_size) {
        // ---
        int tmp_buf[NodeShmCtrl::shm_q_item_size];
        memset(tmp_buf, 0, NodeShmCtrl::shm_q_item_size);

        if (memcmp(tmp_buf, node, NodeShmCtrl::shm_q_item_size) == 0) {
            empty_nodes++;
            continue;
        }

        if (empty_nodes) {
            std::cout << "Empty Queue Slot... x" << empty_nodes << std::endl << std::endl;
            empty_nodes = 0;
        }
        std::cout << "Queue data: " << "\n\t" << std::endl;
        for (int j = 0; j < NodeShmCtrl::shm_q_item_size; ++j) {
            std::cout << node[j] << " ";
            if (j % 20 == 0) { std::cout << std::endl; }
        }
    }
    // Make sure we print any empty node notifications
    if (empty_nodes) {
        std::cout << "Empty Queue Slot... x" << empty_nodes << std::endl << std::endl;
    }
}

}  // namespace fds

int main(int argc, char *argv[]) {
    namespace po = boost::program_options;
    po::options_description desc("Read, and dump shared memory for debugging. Run w/ sudo!");

    desc.add_options()
            ("help,h", "Print this help message")
            ("file,f", po::value<std::string>(),
             "Name of the shared memory file to dump (/dev/shm/\?\?\?).");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    if (vm.count("file") == 0) {
        std::cout << desc << "\n";
        return 1;
    }

    std::string filename = vm["file"].as<std::string>();
    // If there is a /dev/shm/ cut it out
    if (filename.compare(0, 9, "/dev/shm/") == 0) {
        filename = filename.substr(9, std::string::npos);
        std::cout << "Filename is now " << filename << "\n";
    }

    if (filename.find("-rw-") == std::string::npos) {
        fds::ShmDump::read_inv_shm(filename.c_str());
    } else {  // If we're printing a shared memory queue
        fds::ShmDump::read_queue_shm(filename.c_str());
    }

    return 0;
}
