package com.formationds.hadoop;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.XdiService;
import com.formationds.apis.ConfigurationService;

public class FdsPlatform {
    private XdiService.Iface am;
    private ConfigurationService.Iface cs;

    public FdsPlatform(XdiService.Iface am, ConfigurationService.Iface cs) {
        this.am = am;
        this.cs = cs;
    }

    public XdiService.Iface getAm() {
        return am;
    }

    public ConfigurationService.Iface getCs() {
        return cs;
    }
}
