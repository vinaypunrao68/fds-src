package com.formationds.iodriver.events;

import java.time.Duration;
import java.time.Instant;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.operations.GetObjects;

public class PrefixSearch extends Event<PrefixSearch.Data>
{
    public static final class Data
    {
        public Data(GetObjects operation, Duration elapsed, int resultCount)
        {
            if (operation == null) throw new NullArgumentException("operation");
            if (elapsed == null) throw new NullArgumentException("elapsed");
            if (resultCount < 0) throw new IllegalArgumentException("resultCount cannot be < 0.");
            
            _operation = operation;
            _elapsed = elapsed;
            _resultCount = resultCount;
        }
        
        public Duration getElapsed()
        {
            return _elapsed;
        }
        
        public GetObjects getOperation()
        {
            return _operation;
        }
        
        public int getResultCount()
        {
            return _resultCount;
        }
        
        private final Duration _elapsed;
        
        private final GetObjects _operation;
        
        private final int _resultCount;
    }
    
    public PrefixSearch(Instant timestamp, Data data)
    {
        super(timestamp);
        
        if (data == null) throw new NullArgumentException("data");
        
        _data = data;
    }
    
    @Override
    public Data getData()
    {
        return _data;
    }
    
    private final Data _data;
}
