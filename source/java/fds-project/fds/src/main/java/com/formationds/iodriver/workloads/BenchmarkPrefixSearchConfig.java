package com.formationds.iodriver.workloads;

import java.util.HashSet;
import java.util.Set;

import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.RuntimeConfig;
import com.formationds.iodriver.WorkloadProvider;

public class BenchmarkPrefixSearchConfig implements RuntimeConfig
{
    public BenchmarkPrefixSearchConfig()
    {
        _configs = new HashSet<>();
    }

    @Override
    public void addConfig(RuntimeConfig config)
    {
        if (config == null) throw new NullArgumentException("config");
        
        _configs.add(config);
    }

    @Override
    public void addOptions(Options options)
    {
        if (options == null) throw new NullArgumentException("options");
        
        _configs.forEach(c -> c.addOptions(options));
    }
    
    @WorkloadProvider
    public BenchmarkPrefixSearch getBenchmarkPrefixSearchWorkload() throws ParseException
    {
        return new BenchmarkPrefixSearch();
    }

    @Override
    public boolean isHelpNeeded()
    {
        return false;
    }
    
    private final Set<RuntimeConfig> _configs;
}
