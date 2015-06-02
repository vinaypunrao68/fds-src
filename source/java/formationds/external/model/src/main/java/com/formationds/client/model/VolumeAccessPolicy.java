/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.model;

import java.util.Objects;

/**
 *
 */
public class VolumeAccessPolicy {

    public static VolumeAccessPolicy exclusiveReadPolicy() { return new VolumeAccessPolicy(true, false); }
    public static VolumeAccessPolicy exclusiveWritePolicy() { return new VolumeAccessPolicy(false, true); }
    public static VolumeAccessPolicy exclusiveRWPolicy() { return new VolumeAccessPolicy(true, true); }

    private final boolean exclusiveRead;
    private final boolean exclusiveWrite;

    /**
     * @param exclusiveRead restrict reads to exclusive access
     * @param exclusiveWrite restrict writes to exclusive access
     */
    public VolumeAccessPolicy( boolean exclusiveRead, boolean exclusiveWrite ) {
        this.exclusiveRead = exclusiveRead;
        this.exclusiveWrite = exclusiveWrite;
    }

    public boolean isExclusiveRead() { return exclusiveRead; }
    public boolean isExclusiveWrite() { return exclusiveWrite; }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof VolumeAccessPolicy) ) { return false; }
        final VolumeAccessPolicy that = (VolumeAccessPolicy) o;
        return Objects.equals( exclusiveRead, that.exclusiveRead ) &&
               Objects.equals( exclusiveWrite, that.exclusiveWrite );
    }

    @Override
    public int hashCode() {
        return Objects.hash( exclusiveRead, exclusiveWrite );
    }

    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder( "VolumeAccessPolicy{" );
        sb.append( "exclusiveRead=" ).append( exclusiveRead );
        sb.append( ", exclusiveWrite=" ).append( exclusiveWrite );
        sb.append( '}' );
        return sb.toString();
    }
}
