//package com.formationds.iodriver.operations;
//
//import java.util.AbstractMap.SimpleImmutableEntry;
//import java.util.stream.Stream;
//
//import javax.net.ssl.HttpsURLConnection;
//
//import com.formationds.commons.NullArgumentException;
//import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
//import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;
//
//public final class S3OperationWrapper extends OrchestrationManagerOperation
//{
//    public S3OperationWrapper(S3Operation innerOperation)
//    {
//        if (innerOperation == null) throw new NullArgumentException("innerOperation");
//
//        _innerOperation = innerOperation;
//    }
//
//    @Override
//    public void exec(OrchestrationManagerEndpoint endpoint,
//                     HttpsURLConnection connection,
//                     AbstractWorkflowEventListener reporter) throws ExecutionException
//    {
//        throw new UnsupportedOperationException(
//                "Somebody didn't pay attentiont to getNeedsConnection().");
//    }
//
//    @Override
//    public void exec(OrchestrationManagerEndpoint endpoint,
//                     AbstractWorkflowEventListener reporter) throws ExecutionException
//    {
//        if (endpoint == null) throw new NullArgumentException("endpoint");
//        if (reporter == null) throw new NullArgumentException("reporter");
//
//        endpoint.getS3().doVisit(_innerOperation, reporter);
//    }
//
//    @Override
//    public boolean getNeedsConnection()
//    {
//        return false;
//    }
//
//    @Override
//    public String getRequestMethod()
//    {
//        return null;
//    }
//
//    @Override
//    public Stream<SimpleImmutableEntry<String, String>> toStringMembers()
//    {
//        return Stream.concat(super.toStringMembers(),
//                             Stream.of(memberToString("innerOperation", _innerOperation)));
//    }
//
//    private final S3Operation _innerOperation;
//}
