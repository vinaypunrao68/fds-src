package com.formationds.iodriver.workloads;

import java.util.HashSet;
import java.util.Set;

import org.apache.commons.cli.Options;

import com.formationds.commons.RuntimeConfig;

public class BenchmarkPrefixSearchConfig implements RuntimeConfig
{
    public BenchmarkPrefixSearchConfig()
    {
        _configs = new HashSet<>();
    }

    @Override
    public void addConfig(RuntimeConfig config)
    {
        _configs.add(config);
    }

    @Override
    public void addOptions(Options options)
    {
        _configs.forEach(c -> c.addOptions(options));
        
        
    }

    @Override
    public boolean isHelpNeeded()
    {
        return false;
    }
    
    private final Set<RuntimeConfig> _configs;
}
