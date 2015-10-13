package com.formationds.iodriver.reporters;

public class ValidationResult
{
    public ValidationResult(boolean isValid)
    {
        _isValid = isValid;
    }
    
    public final boolean isValid()
    {
        return _isValid;
    }
    
    private final boolean _isValid;
}