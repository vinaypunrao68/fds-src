package com.formationds.sc.api;

import com.formationds.sc.*;
import com.formationds.util.async.ExecutionPolicy;
import com.formationds.util.time.Clock;
import com.formationds.util.time.SystemClock;

import java.util.Collections;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import java.util.function.Function;
import java.util.function.Supplier;

public class FdsChannels {
    private Clock clock;
    private SvcState svc;
    private int dmPrimaryCount = 2;
    private int smPrimaryCount = 2;

    public FdsChannels(SvcState svc) {
        this.svc = svc;
        this.clock = SystemClock.current();
    }

    public void setClock(Clock clock) {
        this.clock = clock;
    }

    public <TResult> CompletableFuture<TResult> dmRead(long volumeId, Function<DmChannel, CompletableFuture<TResult>> operation) {
        return svc.useDmt(dmt ->
                ExecutionPolicy.failover(getDmPrimaries(dmt, volumeId), uuid -> operation.apply(new DmChannel(svc.getChannel(uuid), dmt.getVersion()))
        ));
    }

    public CompletableFuture<Void> dmWrite(long volumeId, Function<DmChannel, CompletableFuture<Void>> operation) {
        return ExecutionPolicy.boundedRetry(defaultTimeout(),
                () ->  svc.useDmt(dmt -> singleDmWrite(dmt, volumeId, operation)));
    }

    public <TResult> CompletableFuture<TResult> smRead(byte[] objectId, Function<SmChannel, CompletableFuture<TResult>> operation) {
        return svc.useDlt(dlt ->
                ExecutionPolicy.failover(getSmPrimaries(dlt, objectId), uuid -> operation.apply(new SmChannel(svc.getChannel(uuid), dlt.getVersion()))));
    }

    public CompletableFuture<Void> smWrite(byte[] objectId, Function<SmChannel, CompletableFuture<Void>> operation) {
        return ExecutionPolicy.boundedRetry(defaultTimeout(),
                () -> svc.useDlt(dlt -> singleSmWrite(dlt, objectId, operation)));
    }


    private CompletableFuture<Void> singleDmWrite(Dmt dmt, long volumeId, Function<DmChannel, CompletableFuture<Void>> operation) {
        return ExecutionPolicy.parallelPrimarySecondary(getDmPrimaries(dmt, volumeId), getDmSecondaries(dmt, volumeId),
                uuid -> {
                    DmChannel channel = new DmChannel(svc.getChannel(uuid), dmt.getVersion());
                    return operation.apply(channel);
                });
    }

    private CompletableFuture<Void> singleSmWrite(Dlt dlt, byte[] objectId, Function<SmChannel, CompletableFuture<Void>> operation) {
        return ExecutionPolicy.parallelPrimarySecondary(getSmPrimaries(dlt, objectId), getSmSecondaries(dlt, objectId),
                uuid -> {
                    SmChannel channel = new SmChannel(svc.getChannel(uuid), dlt.getVersion());
                    return operation.apply(channel);
                });
    }

    private List<Long> getDmPrimaries(Dmt dmt, long volumeId) {
        return primaries(dmt.getDmUuidsForVolumeId(volumeId), dmPrimaryCount);
    }

    private List<Long> getDmSecondaries(Dmt dmt, long volumeId) {
        return secondaries(dmt.getDmUuidsForVolumeId(volumeId), dmPrimaryCount);
    }

    private List<Long> getSmPrimaries(Dlt dlt, byte[] objectId) {
        return primaries(dlt.getSmUuidsForObject(objectId), smPrimaryCount);
    }

    private List<Long> getSmSecondaries(Dlt dlt, byte[] objectId) {
        return secondaries(dlt.getSmUuidsForObject(objectId), smPrimaryCount);
    }

    private List<Long> primaries(long[] uuids, int primaryCount) {
        LongArrayBackedList uuidList = new LongArrayBackedList(uuids);
        return uuidList.subList(0, Math.min(uuids.length, primaryCount));
    }

    private List<Long> secondaries(long[] uuids, int primaryCount) {
        List<Long> dmUuidListForVolumeId = new LongArrayBackedList(uuids);
        if(dmUuidListForVolumeId.size() <= primaryCount)
            return Collections.emptyList();

        return dmUuidListForVolumeId.subList(primaryCount, dmUuidListForVolumeId.size());
    }

    private Supplier<CompletableFuture<Boolean>> defaultTimeout() {
        CompletableFuture<Void> timeout = clock.delay(30, TimeUnit.SECONDS);
        return () -> CompletableFuture.completedFuture(!timeout.isDone());
    }
}
