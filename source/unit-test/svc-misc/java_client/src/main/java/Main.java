/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.protocol.TestAMSvcSync;
import org.apache.thrift.TException;
import org.apache.thrift.async.AsyncMethodCallback;
import org.apache.thrift.async.TAsyncClientManager;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocolFactory;

import java.math.BigDecimal;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;

public class Main {
    public static void main(String[] args) {
        try {
            for (int i = 5; i <= 60; i += 5) {
                ConnectionSpecification cs = new ConnectionSpecification("localhost", 9092);
                TAsyncClientManager manager = new TAsyncClientManager();
                TProtocolFactory factory = new TBinaryProtocol.Factory();
                XdiAsyncResourceImpl<TestAMSvcSync.AsyncIface> impl = new XdiAsyncResourceImpl<>(cs, t -> new TestAMSvcSync.AsyncClient(factory, manager, t));
                AsyncResourcePool<XdiClientConnection<TestAMSvcSync.AsyncIface>> pool = new AsyncResourcePool<>(impl, i);

                int callNumber = 1000000;
                AtomicInteger calls = new AtomicInteger(callNumber);
                ArrayList<CompletableFuture<Long>> data = new ArrayList<>();

                long overallBegin = System.nanoTime();
                for (int j = 0; j < callNumber; j++) {
                    CompletableFuture<Long> execution = pool.use(connection -> {
                        long begin = System.nanoTime();

                        CompletableFuture<Long> result = new CompletableFuture<>();
                        AsyncMethodCallback<TestAMSvcSync.AsyncClient.getBlob_call> callback = new AsyncMethodCallback<TestAMSvcSync.AsyncClient.getBlob_call>() {
                            @Override
                            public void onComplete(TestAMSvcSync.AsyncClient.getBlob_call getBlob_call) {
                                long end = System.nanoTime();
                                try {
//                                    ByteBuffer buf = getBlob_call.getResult();
//                                    if(buf.remaining() != 4096)
//                                        throw new Exception("Result incorrect");
//                                    while(buf.remaining() > 0)
//                                        if(buf.get() != (int)'f')
//                                            throw new Exception("Result incorrect");
                                    result.complete(end - begin);
                                } catch(Exception ex) {
                                    result.completeExceptionally(ex);
                                }
                            }

                            @Override
                            public void onError(Exception e) {
                                result.completeExceptionally(e);
                            }
                        };

                        connection.getClient().getBlob("moops", "boops", "skoops", 10, 100, callback);
                        return result;
                    });

                    data.add(execution);
                }

                ArrayList<Long> latencyMeasurements = new ArrayList<>();
                for(CompletableFuture<Long> dataPoint : data) {
                    latencyMeasurements.add(dataPoint.get());
                }
                long overallEnd = System.nanoTime();
                latencyMeasurements.sort(Long::compare);
                pool.clear();

                BigDecimal callNumberDec = new BigDecimal(callNumber);
                BigDecimal overallTimeNs = new BigDecimal(overallEnd - overallBegin);
                BigDecimal totalLatencyNs = new BigDecimal(latencyMeasurements.stream().collect(Collectors.summingLong(k -> k)));
                BigDecimal reqPerSec = callNumberDec.divide(overallTimeNs.divide(new BigDecimal(1_000_000_000)), BigDecimal.ROUND_CEILING);  // callNumber / (overallTimeNs / 1E9);
                BigDecimal avgLatency = totalLatencyNs.divide(callNumberDec);

                System.out.println(String.format("concurrency: %d  overall: %e ns  th: %e req/s  avgLatency: %e ns", i, overallTimeNs, reqPerSec, avgLatency));
            }
        } catch(Exception ex) {
            System.out.println("Failure: " + ex.toString());
        }
    }
}
