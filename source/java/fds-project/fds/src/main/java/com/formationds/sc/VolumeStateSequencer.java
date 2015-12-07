package com.formationds.sc;

import com.formationds.protocol.VolumeAccessMode;
import com.formationds.protocol.dm.OpenVolumeMsg;
import com.formationds.protocol.dm.OpenVolumeRspMsg;
import com.formationds.sc.api.FdsChannels;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.util.time.Clock;
import com.formationds.util.time.SystemClock;
import org.eclipse.jetty.util.ConcurrentHashSet;

import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;

public class VolumeStateSequencer {
    private Clock clock;
    private FdsChannels channels;
    private static final long REFRESH_TIMEOUT_MS = 30000;
    private static final long USE_TIMEOUT_MS = REFRESH_TIMEOUT_MS * 6;
    private static Map<Long, CompletableFuture<State>> stateMap;

    public VolumeStateSequencer(FdsChannels channels) {
        this.channels = channels;
        this.clock = SystemClock.current();
        stateMap = new HashMap<>();
    }

    public void setClock(Clock clock) {
        this.clock = clock;
    }

    public CompletableFuture<SequenceData> useVolume(long volumeId) {
        return openOrGetVolume(volumeId).thenApply(f -> f.nextSequenceId());
    }

    private CompletableFuture<State> openOrGetVolume(long volumeId) {
        CompletableFuture<State> state = null;
        boolean isNew = false;
        synchronized (stateMap) {
            state = stateMap.get(volumeId);
            if(state == null || state.isCompletedExceptionally()) {
                state = new CompletableFuture<>();
                isNew = true;
            }
        }

        if(isNew) {
            CompletableFutureUtility.tie(openVolumeInternal(volumeId), state);
            scheduleRefresh(volumeId);
        }

        Optional<State> s = CompletableFutureUtility.getNowIfCompletedNormally(state);
        s.ifPresent(st -> st.setLastUse(clock.currentTimeMillis()));

        return state;
    }

    private void scheduleRefresh(long volumeId) {
        clock.delay(REFRESH_TIMEOUT_MS, TimeUnit.MILLISECONDS).whenComplete((r, ex) -> refresh(volumeId));
    }

    private CompletableFuture<State> openVolumeInternal(long volumeId) {
        ConcurrentHashSet<OpenVolumeRspMsg> responses = new ConcurrentHashSet<>();
        return channels.dmWrite(volumeId, dm -> {
            VolumeAccessMode vam = new VolumeAccessMode();
            vam.setCan_write(true);
            vam.setCan_cache(true);
            return dm.openVolume(new OpenVolumeMsg(volumeId, vam))
                    .thenAccept(msg -> responses.add(msg));
        }).thenCompose(_null -> makeStateFromResults(responses));
    }

    private void refresh(long volumeId) {
        long now = clock.currentTimeMillis();
        CompletableFuture<State> state = null;
        synchronized (stateMap) {
            state = stateMap.get(volumeId);
        }

        if(state != null) {
            // handle timeout
            Optional<State> current = CompletableFutureUtility.getNowIfCompletedNormally(state);
            if(current.isPresent() && (current.get().lastUse.get() + USE_TIMEOUT_MS) < now) {
                synchronized (stateMap) {
                    stateMap.remove(volumeId);
                    return;
                }
            }
        }

        // refresh lease
        openVolumeInternal(volumeId).whenComplete((r, ex) -> {
            if (ex != null) {
                synchronized (stateMap) {
                    stateMap.remove(volumeId);
                }
            } else {
                scheduleRefresh(volumeId);
            }
        });
    }

    private CompletableFuture<State> makeStateFromResults(Set<OpenVolumeRspMsg> responses) {
        OpenVolumeRspMsg firstResponse = null;
        for(OpenVolumeRspMsg response : responses) {
            if(firstResponse == null)
                firstResponse = response;
            else if(firstResponse.sequence_id != response.sequence_id || firstResponse.token != response.token)
                return CompletableFutureUtility.exceptionFuture(new Exception("Non-uniform open response from DM"));
        }

        if(firstResponse == null)
            CompletableFutureUtility.exceptionFuture(new Exception("No valid responses to open msg from DM"));

        return CompletableFuture.completedFuture(new State(firstResponse.sequence_id, firstResponse.token, clock.currentTimeMillis()));
    }

    public class SequenceData {
        private long sequenceId;
        private long token;

        SequenceData(long sequenceId, long token) {
            this.sequenceId = sequenceId;
            this.token = token;
        }

        public long getSequenceId() {
            return sequenceId;
        }

        public long getToken() {
            return token;
        }
    }

    private class State {
        private AtomicLong lastUse;
        private AtomicLong sequenceId;
        private long token;

        public State(long sequence_id, long token, long now) {
            this.sequenceId = new AtomicLong(sequence_id);
            this.token = token;
            this.lastUse = new AtomicLong(now);
        }

        public SequenceData nextSequenceId() {
            return new SequenceData(sequenceId.getAndIncrement(), token);
        }

        public void setLastUse(long time) {
            lastUse.accumulateAndGet(time, (current, updated) -> Math.max(current, updated));
        }
    }

}
