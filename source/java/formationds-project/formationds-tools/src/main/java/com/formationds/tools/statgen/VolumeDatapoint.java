/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.tools.statgen;

import com.google.gson.annotations.SerializedName;

import java.util.Objects;

/**
 * @author ptinius
 */
// TODO: copied from commons model for now.   Need to move to external model, most likely in new format
public class VolumeDatapoint {
    private static final long serialVersionUID = -528746171551767393L;

    private Integer id;
    @SerializedName("timestamp")
    private Long    timestamp;
    @SerializedName("volume")
    private String  volumeName;
    @SerializedName("volumeId")
    private Long    volumeId;

    // TODO: why aren't we using the Metrics enum with @Enumerated here?
    private String key;
    private Double value;

    /**
     *
     */
    public VolumeDatapoint() {
    }

    /**
     * @param timestamp  the datapoint timestamp
     * @param volumeId   the volume id
     * @param volumeName the volume name
     * @param key        the metric key
     * @param value      the metric value
     */
    public VolumeDatapoint( Long timestamp, Long volumeId, String volumeName, String key, Double value ) {
        this.timestamp = timestamp;
        this.volumeName = volumeName;
        this.volumeId = volumeId;
        this.key = key;
        this.value = value;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof VolumeDatapoint) ) { return false; }

        VolumeDatapoint that = (VolumeDatapoint) o;

        if ( !timestamp.equals( that.timestamp ) ) { return false; }
        if ( !volumeId.equals( that.volumeId ) ) { return false; }
        if ( !volumeName.equals( that.volumeName ) ) { return false; }
        if ( !key.equals( that.key ) ) { return false; }

        return true;
    }

    @Override
    public int hashCode() {
        return Objects.hash( timestamp, volumeId, volumeName, key );
    }

    /**
     * @return Returns the auto-generated id
     */
    public Integer getId() {
        return id;
    }

    /**
     * @return Returns the {@code double} representing the statistic value for
     * the {@code key}
     */
    public Double getValue() {
        return value;
    }

    /**
     * @param value the {@code double} representing the statistic value for the
     *              {@code key}
     */
    public void setValue( final Double value ) {
        this.value = value;
    }

    /**
     * @return Returns the {@code Long} representing the timestamp in seconds since the epoch
     */
    public Long getTimestamp() {
        return timestamp;
    }

    /**
     * @param timestamp the {@code long} representing the timestamp in seconds since the epoch
     */
    public void setTimestamp( final long timestamp ) {
        this.timestamp = timestamp;
    }

    /**
     * @return Returns the {@link String} representing the volume name
     */
    public String getVolumeName() {
        return volumeName;
    }

    /**
     * @param volumeName the {@link String} representing the volume name
     */
    public void setVolumeName( final String volumeName ) {
        this.volumeName = volumeName;
    }

    /**
     * @return Returns {@link String} representing the volume id
     */
    public Long getVolumeId() {
        return volumeId;
    }

    /**
     * @param volumeId the {@link String} representing the volume id
     */
    public void setVolumeId( final Long volumeId ) {
        this.volumeId = volumeId;
    }

    /**
     * @return Returns the {@link String} representing the metadata key
     */
    public String getKey() {
        return key;
    }

    /**
     * @param key the {@link String} representing the metadata key
     */
    public void setKey( final String key ) {
        this.key = key;
    }

    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder( "VolumeDatapoint{" );
        sb.append( "id=" ).append( id )
          .append( ", timestamp=" ).append( timestamp )
          .append( ", volumeName='" ).append( volumeName ).append( '\'' )
          .append( ", volumeId='" ).append( volumeId ).append( '\'' )
          .append( ", key='" ).append( key ).append( '\'' )
          .append( ", value=" ).append( value )
          .append( '}' );
        return sb.toString();
    }
}
