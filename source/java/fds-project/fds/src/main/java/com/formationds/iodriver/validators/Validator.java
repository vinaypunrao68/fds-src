package com.formationds.iodriver.validators;

import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * A class that implements this interface will validate the statistics gathered by running a
 * workload.
 */
@FunctionalInterface
public interface Validator
{
    /**
     * Determine if a given workload run should be considered "valid". There is no definition for
     * valid, beyond that {@code true} is returned.
     * 
     * @param listener The event listener with the stats to analyze.
     * 
     * @return Whether this run should be accepted.
     */
    boolean isValid(AbstractWorkflowEventListener listener);
}
