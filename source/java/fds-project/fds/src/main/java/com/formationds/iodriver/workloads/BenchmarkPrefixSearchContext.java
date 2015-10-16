package com.formationds.iodriver.workloads;

import com.formationds.commons.patterns.Subject;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.events.PrefixSearch;

public final class BenchmarkPrefixSearchContext extends WorkloadContext
{
    public BenchmarkPrefixSearchContext(Logger logger)
    {
        super(logger);
    }
    
    @Override
    protected void setUp()
    {
        super.setUp();
        
        Subject<PrefixSearch> prefixSearch = new Subject<>();
        
        registerEvent(PrefixSearch.class, prefixSearch);
    }
}
