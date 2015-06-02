/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.time.Instant;
import java.util.Objects;

// Immutable/Thread-safe
public class VolumeStatus {
    private final Instant     timestamp;
    private final VolumeState state;
    private final Size        currentUsage;
    private final Instant     lastCapacityFirebreak;
    private final Instant     lastPerformanceFirebreak;

    /**
     * Volume status with the specified state and space usage using a timestamp
     * defaulting to now and no firebreaks detected.
     *
     * @param state the state
     * @param currentUsage the current space usage
     */
    public VolumeStatus( VolumeState state, Size currentUsage ) {
        this( Instant.now(), state, currentUsage, Instant.MIN, Instant.MIN);
    }

    /**
     * Volume status with the specified state and space usage using a timestamp
     * defaulting to now.
     *
     * @param state the state
     * @param currentUsage the current space usage
     * @param lastCapacityFirebreak the timestamp of the last detected capacity firebreak, or Instant.MIN (0) if none
     * @param lastPerformanceFirebreak the timestamp of the last detected performance firebreak or Instant.MIN (0) if none
     */
    public VolumeStatus( VolumeState state, Size currentUsage, Instant lastCapacityFirebreak,
                         Instant lastPerformanceFirebreak ) {
        this(Instant.now(), state, currentUsage, lastCapacityFirebreak, lastPerformanceFirebreak);
    }

    /**
     * Volume status with the specified state and space usage using the specified timestamp
     *
     * @param timestamp the timestamp the volume status snapshot was taken
     * @param state the state
     * @param currentUsage the current space usage
     * @param lastCapacityFirebreak the timestamp of the last detected capacity firebreak, or Instant.MIN (0) if none
     * @param lastPerformanceFirebreak the timestamp of the last detected performance firebreak or Instant.MIN (0) if none
     */
    public VolumeStatus( Instant timestamp, VolumeState state, Size currentUsage, Instant lastCapacityFirebreak,
                         Instant lastPerformanceFirebreak ) {
        this.timestamp = timestamp;
        this.state = state;
        this.currentUsage = currentUsage;
        this.lastCapacityFirebreak = lastCapacityFirebreak;
        this.lastPerformanceFirebreak = lastPerformanceFirebreak;
    }

    boolean hasCapacityFirebreak() { return !Instant.MIN.equals( lastCapacityFirebreak ); }
    boolean hasPerformanceFirebreak() { return !Instant.MIN.equals( lastPerformanceFirebreak ); }
    boolean hasFirebreak() { return hasCapacityFirebreak() || hasPerformanceFirebreak(); }

    public Instant getTimestamp() {
        return timestamp;
    }

    public VolumeState getState() {
        return state;
    }

    public Size getCurrentUsage() {
        return currentUsage;
    }

    public Instant getLastCapacityFirebreak() {
        return lastCapacityFirebreak;
    }

    public Instant getLastPerformanceFirebreak() {
        return lastPerformanceFirebreak;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof VolumeStatus) ) { return false; }
        final VolumeStatus that = (VolumeStatus) o;
        return Objects.equals( timestamp, that.timestamp ) &&
               Objects.equals( state, that.state ) &&
               Objects.equals( currentUsage, that.currentUsage ) &&
               Objects.equals( lastCapacityFirebreak, that.lastCapacityFirebreak ) &&
               Objects.equals( lastPerformanceFirebreak, that.lastPerformanceFirebreak );
    }

    @Override
    public int hashCode() {
        return Objects.hash( timestamp, state, currentUsage, lastCapacityFirebreak, lastPerformanceFirebreak );
    }

    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder( "VolumeStatus{" );
        sb.append( "timestamp=" ).append( timestamp );
        sb.append( ", state=" ).append( state );
        sb.append( ", currentUsage=" ).append( currentUsage );
        sb.append( ", lastCapacityFirebreak=" ).append( lastCapacityFirebreak );
        sb.append( ", lastPerformanceFirebreak=" ).append( lastPerformanceFirebreak );
        sb.append( '}' );
        return sb.toString();
    }
}
