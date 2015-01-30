package com.formationds.iodriver.endpoints;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.patterns.Visitor;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.VerificationReporter;

public abstract class Endpoint<ThisT extends Endpoint<ThisT, OperationT>, OperationT extends Operation<OperationT, ThisT>>
                                                                                                                           implements
                                                                                                                           Visitor<OperationT, ThisT, ExecutionException>
{
    public abstract ThisT copy();

    public abstract void doVisit(OperationT operation) throws ExecutionException;

    protected abstract class CopyHelper
    {
        public final VerificationReporter reporter = _reporter;
    }

    protected Endpoint(VerificationReporter reporter)
    {
        if (reporter == null) throw new NullArgumentException("reporter");
        
        _reporter = reporter;
    }

    protected Endpoint(CopyHelper helper)
    {
        if (helper == null) throw new NullArgumentException("helper");
        
        _reporter = helper.reporter;
    }
    
    protected final VerificationReporter getReporter()
    {
        return _reporter;
    }
    
    private final VerificationReporter _reporter;
}
