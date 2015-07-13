//package com.formationds.iodriver.operations;
//
//import com.formationds.commons.NullArgumentException;
//import com.formationds.iodriver.endpoints.FdsEndpoint;
//import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;
//
//public class FdsS3OperationWrapper extends FdsOperation
//{
//    public FdsS3OperationWrapper(S3Operation innerOperation)
//    {
//        if (innerOperation == null) throw new NullArgumentException("innerOperation");
//        
//        _innerOperation = innerOperation;
//    }
//    
//    @Override
//    public void accept(FdsEndpoint endpoint,
//                       AbstractWorkflowEventListener reporter) throws ExecutionException
//    {
//        if (endpoint == null) throw new NullArgumentException("endpoint");
//        if (reporter == null) throw new NullArgumentException("reporter");
//        
//        endpoint.getS3Endpoint().doVisit(_innerOperation, reporter);
//    }
//    
//    private final S3Operation _innerOperation;
//}
