package com.formationds.iodriver.reporters;

@FunctionalInterface
public interface Validator
{
    boolean isValid(WorkflowEventListener listener);
}
