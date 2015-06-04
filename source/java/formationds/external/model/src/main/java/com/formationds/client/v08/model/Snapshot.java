package com.formationds.client.v08.model;

import java.time.Duration;
import java.time.Instant;
import java.util.Objects;

public class Snapshot extends AbstractResource<Long> {

    private final Duration retention;
    private final Long     volumeId;
    private final Instant  creationTime;

    private Snapshot() {
        this( 0L, "None", 0L, null, null );
    }

    public Snapshot( Long id, String name ) {
        this( id, name, 0L, null, null );
    }

    public Snapshot( Long id, String name, Long volumeId, Duration retention, Instant creationTime ) {
        super( id, name );

        this.volumeId = volumeId;
        this.retention = retention;
        this.creationTime = creationTime;
    }

    public Duration getRetention() {
        return this.retention;
    }

    public Long getVolumeId() {
        return this.volumeId;
    }

    public Instant getCreationTime() {
        return this.creationTime;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof Snapshot) ) { return false; }
        if ( !super.equals( o ) ) { return false; }
        final Snapshot snapshot = (Snapshot) o;
        return Objects.equals( retention, snapshot.retention ) &&
               Objects.equals( volumeId, snapshot.volumeId ) &&
               Objects.equals( creationTime, snapshot.creationTime );
    }

    @Override
    public int hashCode() {
        return Objects.hash( super.hashCode(), retention, volumeId, creationTime );
    }

    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder( "Snapshot{" );
        sb.append( "retention=" ).append( retention );
        sb.append( ", volumeId=" ).append( volumeId );
        sb.append( ", creationTime=" ).append( creationTime );
        sb.append( '}' );
        return sb.toString();
    }
}
