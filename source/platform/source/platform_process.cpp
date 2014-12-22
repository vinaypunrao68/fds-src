/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <string>

#include <platform/platform-lib.h>
#include <platform/platform_process.h>
#include <platform/platform.h>

namespace fds
{
    // -------------------------------------------------------------------------------------
    // FDS Platform Process
    // -------------------------------------------------------------------------------------

    // Table of keys used to access the persistent storage DB.
    //

    /**
     * @brief Constructor.  Keep it as bare shell.  Do the initialization work
     * in init()
     */
    PlatformProcess::PlatformProcess()
    {
    }

    /**
     * @brief Constructor.  Keep it as bare shell.  Do the initialization work
     *
     * @param argc
     * @param argv
     * @param cfg
     * @param log
     * @param platform
     * @param vec
     *
     * @return
     */
    PlatformProcess::PlatformProcess(int argc, char **argv, const std::string  &cfg,
                                     const std::string  &log, Platform           *platform,
                                     Module            **vec)
    {
        init(argc, argv, cfg, log, platform, vec);
    }

    /**
     * @brief Common method for constructing Platform process object
     *
     * @param argc
     * @param argv
     * @param cfg
     * @param log
     * @param platform
     * @param vec
     */
    void PlatformProcess::init(int argc, char **argv, const std::string  &cfg,
                               const std::string  &log, Platform           *platform,
                               Module            **vec)
    {
        FdsProcess::init(argc, argv, "platform.conf", cfg, log, vec);
        plf_mgr = platform;
        plf_db = NULL;
        plf_test_mode = false;
        plf_stand_alone = false;

        memset(&plf_node_data, 0, sizeof(plf_node_data));
        plf_db_key = proc_root->dir_fdsroot() + "node-root";
        // TODO(Bao): daemonize();  make each platform process a daemon to get core.
    }

    PlatformProcess::~PlatformProcess()
    {
        delete plf_db;
    }

    // plf_load_node_data
    // ------------------
    //
    void PlatformProcess::plf_load_node_data()
    {
        if ((plf_test_mode == true) || (plf_stand_alone == true))
        {
            return;
        }

        if (plf_db->isConnected() == false)
        {
            // TODO(Vy): platform daemon should start redis server here...
            //
            LOGNORMAL << "Sorry, you must start redis server manually & try again" << std::endl;
            exit(1);
        }
        std::string    val = plf_db->get(plf_db_key);

        if (val == "")
        {
            // The caller will have to deal with it.
            return;
        }
        memcpy(&plf_node_data, val.data(), sizeof(plf_node_data));

        // TODO(Vy): consistency check on the data read.
        //
    }

    // plf_apply_node_data
    // -------------------
    //
    void PlatformProcess::plf_apply_node_data()
    {
        plf_mgr->plf_change_info(&plf_node_data);
    }

    void PlatformProcess::proc_pre_startup()
    {
        plf_db = new kvstore::PlatformDB();
        plf_load_node_data();
        plf_apply_node_data();
    }
}  // namespace fds
