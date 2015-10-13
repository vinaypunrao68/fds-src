package com.formationds.iodriver.events;

import java.time.Instant;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.reporters.ValidationResult;

public class Validated extends Event<ValidationResult>
{
    public Validated(Instant timestamp, ValidationResult result)
    {
        super(timestamp);
        
        if (result == null) throw new NullArgumentException("result");
        
        _result = result;
    }
    
    @Override
    public ValidationResult getData()
    {
        return _result;
    }
    
    private final ValidationResult _result;
}
