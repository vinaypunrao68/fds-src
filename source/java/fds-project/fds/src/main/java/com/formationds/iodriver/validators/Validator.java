package com.formationds.iodriver.validators;

import java.io.Closeable;

import com.formationds.iodriver.reporters.WorkloadEventListener;

/**
 * A class that implements this interface will validate the statistics gathered by running a
 * workload.
 */
public interface Validator
{
    /**
     * Determine if a given workload run should be considered "valid". There is no definition for
     * valid, beyond that {@code true} is returned.
     * 
     * @param Context Collection of statistics to analyze.
     * 
     * @return Whether this run should be accepted.
     */
    boolean isValid(Closeable context, WorkloadEventListener listener);

    Closeable newContext(WorkloadEventListener listener);
}
