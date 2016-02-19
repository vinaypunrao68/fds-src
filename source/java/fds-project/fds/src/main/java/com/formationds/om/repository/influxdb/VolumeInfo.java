package com.formationds.om.repository.influxdb;

import java.util.Objects;

// TODO: add tenant name/id...
public class VolumeInfo {

    final String domainName;
    final String volumeName;
    final Long volumeId;
    VolumeInfo(Long volumeId) {
        this("", null, volumeId);
    }
    public VolumeInfo( String domainName, String volumeName, Long volumeId ) {
        super();
        this.domainName = domainName;
        this.volumeName = volumeName;
        this.volumeId = volumeId;
    }
    /**
     * @return the domainName
     */
    public String getDomainName() {
        return domainName;
    }
    /**
     * @return the volumeName
     */
    public String getVolumeName() {
        return volumeName;
    }
    /**
     * @return the volumeId
     */
    public Long getVolumeId() {
        return volumeId;
    }
    /* (non-Javadoc)
     * @see java.lang.Object#hashCode()
     */
    @Override
    public int hashCode() {
        return Objects.hash( domainName, volumeName, volumeId );
    }

    /* (non-Javadoc)
     * @see java.lang.Object#equals(java.lang.Object)
     */
    @Override
    public boolean equals( Object obj ) {
        if ( this == obj )
            return true;
        if ( obj == null )
            return false;
        if ( getClass() != obj.getClass() )
            return false;
        VolumeInfo other = (VolumeInfo) obj;
        return Objects.equals( domainName, other.getDomainName() ) &&
                Objects.equals( volumeName, other.getVolumeName() ) &&
                Objects.equals( volumeId, other.getVolumeId() );
    }

    /* (non-Javadoc)
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return String.format( "VolumeInfo [domainName=%s, volumeName=%s, volumeId=%s]",
                              domainName, volumeName, volumeId );
    }
}