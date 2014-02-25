/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_PLATFORM_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_PLATFORM_H_

#include <platform/platform-lib.h>

namespace fds {

class DmVolEvent : public PlatEvent
{
  public:
    typedef boost::intrusive_ptr<DmVolEvent> pointer;
    typedef boost::intrusive_ptr<const DmVolEvent> const_ptr;

    virtual ~DmVolEvent() {}
    DmVolEvent() : PlatEvent("DM-VolEvent") {}

    virtual void plat_evt_handler();
};

class DmPlatform : public Platform
{
  public:
    DmPlatform();
    virtual ~DmPlatform() {}

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();
};

extern DmPlatform gl_DmPlatform;

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_PLATFORM_H_
