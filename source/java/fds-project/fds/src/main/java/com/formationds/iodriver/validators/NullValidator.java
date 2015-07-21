package com.formationds.iodriver.validators;

import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * Doesn't actually validate anything.
 */
public class NullValidator implements Validator
{
    @Override
    public boolean isValid(AbstractWorkflowEventListener listener)
    {
        return true;
    }
}
