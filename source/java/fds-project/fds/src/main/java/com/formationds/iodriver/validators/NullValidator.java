package com.formationds.iodriver.validators;

import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;

/**
 * Doesn't actually validate anything.
 */
public class NullValidator implements Validator
{
    @Override
    public boolean isValid(AbstractWorkloadEventListener listener)
    {
        return true;
    }
}
