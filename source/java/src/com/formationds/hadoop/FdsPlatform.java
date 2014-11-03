package com.formationds.hadoop;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;

public class FdsPlatform {
    private AmService.Iface am;
    private ConfigurationService.Iface cs;

    public FdsPlatform(AmService.Iface am, ConfigurationService.Iface cs) {
        this.am = am;
        this.cs = cs;
    }

    public AmService.Iface getAm() {
        return am;
    }

    public ConfigurationService.Iface getCs() {
        return cs;
    }
}
