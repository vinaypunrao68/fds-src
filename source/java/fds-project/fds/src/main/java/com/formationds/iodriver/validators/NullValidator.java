package com.formationds.iodriver.validators;

import com.formationds.iodriver.WorkloadContext;

/**
 * Doesn't actually validate anything.
 */
public final class NullValidator implements Validator
{
    @Override
    public boolean isValid(WorkloadContext context)
    {
        return true;
    }
}
