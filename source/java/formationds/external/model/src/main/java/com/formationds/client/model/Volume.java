/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.model;

import com.formationds.client.ical.RecurrenceRule;
import com.formationds.client.model.ID.FdsUID;
import com.formationds.client.model.Volume.QosPolicyBase;

import java.time.Duration;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

public class Volume extends AbstractResource {

    public static abstract  class SnapshotPolicyBase {
        private ID             id;
        private Duration       retentionTime;
        private RecurrenceRule recurrenceRule;

        public SnapshotPolicyBase( Duration retentionTime, RecurrenceRule recurrenceRule ) {
            this.retentionTime = retentionTime;
            this.recurrenceRule = recurrenceRule;
        }

        public SnapshotPolicyBase( ID id, Duration retentionTime, RecurrenceRule recurrenceRule ) {
            this.id = id;
            this.retentionTime = retentionTime;
            this.recurrenceRule = recurrenceRule;
        }

        public ID getId() {
            return id;
        }

        public SnapshotPolicyBase withId( ID id ) {
            this.id = id;
            return this;
        }

        public Duration getRetentionTime() {
            return retentionTime;
        }

        public SnapshotPolicyBase withRetentionTime( Duration retentionTime ) {
            this.retentionTime = retentionTime;
            return this;
        }

        public RecurrenceRule getRecurrenceRule() {
            return recurrenceRule;
        }

        public SnapshotPolicyBase withRecurrenceRule( RecurrenceRule recurrenceRule ) {
            this.recurrenceRule = recurrenceRule;
            return this;
        }
    }

    public static class SnapshotPolicy extends SnapshotPolicyBase {

        public static SnapshotPolicy fromPreset(SnapshotPolicyBase p) {
            return new SnapshotPolicy( p.getId(), p.getRecurrenceRule(), p.getRetentionTime() );
        }

        private ID presetId;

        public SnapshotPolicy( RecurrenceRule recurrenceRule, Duration retentionTime ) {
            super(retentionTime, recurrenceRule);
        }

        public SnapshotPolicy( ID presetId, RecurrenceRule recurrenceRule, Duration retentionTime ) {
            super(retentionTime, recurrenceRule);
            this.presetId = presetId;
        }

        public SnapshotPolicy( ID id, ID presetId, RecurrenceRule recurrenceRule, Duration retentionTime ) {
            super(id, retentionTime, recurrenceRule );
            this.presetId = presetId;
        }

        public ID getPresetId() { return presetId; }

        public SnapshotPolicy withPresetId( ID presetId ) {
            this.presetId = presetId;
            return this;
        }
    }

    public static class SnapshotPolicyPreset extends SnapshotPolicyBase {

        public SnapshotPolicyPreset( Duration retentionTime, RecurrenceRule recurrenceRule ) {
            super(retentionTime, recurrenceRule);
        }

        public SnapshotPolicyPreset( ID id, Duration retentionTime, RecurrenceRule recurrenceRule ) {
            super(id, retentionTime, recurrenceRule);
        }
    }

    public static abstract class DataProtectionPolicyBase {

        private ID id;
        private Duration commitLogRetention;
        private List<SnapshotPolicy> snapshotPolicies;

        public DataProtectionPolicyBase( Duration commitLogRetention,
                                         List<SnapshotPolicy> snapshotPolicies ) {
            this.commitLogRetention = commitLogRetention;
            this.snapshotPolicies = new ArrayList<>( snapshotPolicies );
        }

        public DataProtectionPolicyBase( ID id,
                                         Duration commitLogRetention,
                                         List<SnapshotPolicy> snapshotPolicies ) {
            this.id = id;
            this.commitLogRetention = commitLogRetention;
            this.snapshotPolicies = new ArrayList<>( snapshotPolicies );
        }

        public ID getId() { return id; }
        public Duration getCommitLogRetention() { return commitLogRetention; }
        public List<SnapshotPolicy> getSnapshotPolicies() { return snapshotPolicies; }

        public void setId( ID id ) {
            this.id = id;
        }

        public void setCommitLogRetention( Duration commitLogRetention ) {
            this.commitLogRetention = commitLogRetention;
        }

        public void setSnapshotPolicies( List<SnapshotPolicy> snapshotPolicies ) {
            this.snapshotPolicies = new ArrayList<>( snapshotPolicies );
        }

        @Override
        public boolean equals( Object o ) {
            if ( this == o ) { return true; }
            if ( !(o instanceof DataProtectionPolicyBase) ) { return false; }
            final DataProtectionPolicyBase that = (DataProtectionPolicyBase) o;
            return Objects.equals( id, that.id ) &&
                   Objects.equals( commitLogRetention, that.commitLogRetention ) &&
                   Objects.equals( snapshotPolicies, that.snapshotPolicies );
        }

        @Override
        public int hashCode() {
            return Objects.hash( id, commitLogRetention, snapshotPolicies );
        }
    }

    public static class DataProtectionPolicyPreset extends DataProtectionPolicyBase {
        public DataProtectionPolicyPreset( ID id, Duration commitLogRetention,
                                           List<SnapshotPolicy> snapshotPolicies ) {
            super( id, commitLogRetention, snapshotPolicies );
        }
    }

    public static class DataProtectionPolicy extends DataProtectionPolicyBase {
        public static DataProtectionPolicy fromPreset(DataProtectionPolicyPreset p) {
            return new DataProtectionPolicy( p.getId(), p.getCommitLogRetention(), p.getSnapshotPolicies() );
        }

        private ID presetId = null;
        private Volume volume;
        public DataProtectionPolicy( Duration commitLogRetention,
                                     List<SnapshotPolicy> snapshotPolicies ) {
            super(commitLogRetention, snapshotPolicies);
        }
        public DataProtectionPolicy( ID presetId, Duration commitLogRetention,
                                     List < SnapshotPolicy > snapshotPolicies ) {
            super( commitLogRetention, snapshotPolicies );
            this.presetId = presetId;
        }

        public DataProtectionPolicy( ID id, ID presetId, Duration commitLogRetention,
                                     List < SnapshotPolicy > snapshotPolicies ) {
            super( id, commitLogRetention, snapshotPolicies );
            this.presetId = presetId;
        }

        public ID getPresetId() {
            return presetId;
        }

        public void setPresetId( ID presetId ) {
            this.presetId = presetId;
        }

        @Override
        public boolean equals( Object o ) {
            if ( this == o ) { return true; }
            if ( !(o instanceof DataProtectionPolicy) ) { return false; }
            if ( !super.equals( o ) ) { return false; }
            final DataProtectionPolicy that = (DataProtectionPolicy) o;
            return Objects.equals( presetId, that.presetId );
        }

        @Override
        public int hashCode() {
            return Objects.hash( super.hashCode(), presetId );
        }
    }

    public static class QosPolicy extends QosPolicyBase {

        public static QosPolicy fromPreset(QosPolicyPreset p) {
            return new QosPolicy( p.getId(), p.getPriority(), p.getIopsMin(), p.getIopsMax() );
        }

        private ID presetID;

        public QosPolicy( int priority, int iopsMin, int iopsMax ) {
            super(priority, iopsMin, iopsMax );
        }

        public QosPolicy( ID presetID, int priority, int iopsMin, int iopsMax ) {
            super(priority, iopsMin, iopsMax );
            this.presetID = presetID;
        }

        public QosPolicy( ID id, ID presetID, int priority, int iopsMin, int iopsMax ) {
            super(id, priority, iopsMin, iopsMax );
            this.presetID = presetID;
        }

        public ID getPresetID() {
            return presetID;
        }

        public void setPresetID( ID presetID ) {
            this.presetID = presetID;
        }
    }

    public static class QosPolicyBase {

        // id generated when the policy preset is saved.  null indicates an unsaved preset policy
        private final ID id;

        private final int priority;
        private final int iopsMin;
        private final int iopsMax;

        /**
         *
         * @param id the id of the qos policy preset
         * @param priority the priority
         * @param iopsMin the minimum iops setting
         * @param iopsMax the maximum iops setting
         */
        QosPolicyBase( ID id, int priority, int iopsMin, int iopsMax ) {
            this.id = id;
            this.priority = priority;
            this.iopsMin = iopsMin;
            this.iopsMax = iopsMax;
        }

        /**
         *
         * @param priority the priority
         * @param iopsMin the minimum iops setting
         * @param iopsMax the maximum iops setting
         */
        public QosPolicyBase( int priority, int iopsMin, int iopsMax ) {
            this.id = null;
            this.priority = priority;
            this.iopsMin = iopsMin;
            this.iopsMax = iopsMax;
        }

        public ID getId() { return id; }
        public int getPriority() {  return priority; }
        public int getIopsMin() { return iopsMin; }
        public int getIopsMax() { return iopsMax; }

        /**
         *
         * @param id the id for the qos policy preset
         * @return a new instance with the specified ID and the settings for this policy
         */
        QosPolicyPreset withId(Long id) {
            if (this.id != null) {
                throw new IllegalStateException( "Only generate ID for presets that do not yet have an ID");
            }
            return new QosPolicyPreset( FdsUID.id( id ), this.priority, this.iopsMin, this.iopsMax );
        }

        public boolean idEquals( Object o ) {
            if ( this == o ) { return true; }
            if ( !(o instanceof QosPolicyBase) ) { return false; }
            final QosPolicyBase that = (QosPolicyBase) o;
            return Objects.equals( id, that.id );

        }

        @Override
        public boolean equals( Object o ) {
            if ( this == o ) { return true; }
            if ( !(o instanceof QosPolicyBase) ) { return false; }
            final QosPolicyBase that = (QosPolicyBase) o;
            return Objects.equals( priority, that.priority ) &&
                   Objects.equals( iopsMin, that.iopsMin ) &&
                   Objects.equals( iopsMax, that.iopsMax );
        }

        @Override
        public int hashCode() {
            return Objects.hash( priority, iopsMin, iopsMax );
        }

        @Override
        public String toString() {
            final StringBuilder sb = new StringBuilder( "QosPolicyBase{" );
            sb.append( "id=" ).append( id );
            sb.append( ", priority=" ).append( priority );
            sb.append( ", iopsMin=" ).append( iopsMin );
            sb.append( ", iopsMax=" ).append( iopsMax );
            sb.append( '}' );
            return sb.toString();
        }
    }

    public static class QosPolicyPreset extends QosPolicyBase {
        /**
         *
         * @param id the id of the qos policy preset
         * @param priority the priority
         * @param iopsMin the minimum iops setting
         * @param iopsMax the maximum iops setting
         */
        QosPolicyPreset( ID id, int priority, int iopsMin, int iopsMax ) {
            super(id, priority, iopsMin, iopsMax );
        }

        /**
         *
         * @param priority the priority
         * @param iopsMin the minimum iops setting
         * @param iopsMax the maximum iops setting
         */
        public QosPolicyPreset( int priority, int iopsMin, int iopsMax ) {
            super(priority, iopsMin, iopsMax );
        }

    }

    public static class Builder {

        private final String volumeName;
        private final Tenant              tenant;
        private final Map<String, String> tags = new HashMap<>();
        private String               application;
        private VolumeStatus         status;
        private VolumeSettings       settings;
        private MediaPolicy          mediaPolicy;
        private DataProtectionPolicy dataProtectionPolicy;
        private VolumeAccessPolicy   accessPolicy;
        private QosPolicy            qosPolicy;

        public Builder( Tenant tenant, String volumeName ) {
            this.tenant = tenant;
            this.volumeName = volumeName;
        }

        public Builder addTag(String k, String v) { this.tags.put(k, v); return this; }
        public Builder addTags(Map<String,String> tags) { this.tags.putAll(tags); return this; }
        public Builder application(String app) { this.application = app; return this; }
        public Builder status(VolumeStatus status) { this.status = status; return this; }
        public Builder mediaPolicy(MediaPolicy m) { this.mediaPolicy = m; return this; }
        public Builder accessPolicy(VolumeAccessPolicy ap) { this.accessPolicy = ap; return this; }
        public Builder qosPolicy(QosPolicy qp) { this.qosPolicy = qp; return this; }

        public Builder dataProtectionPolicy(DataProtectionPolicy dpp) { this.dataProtectionPolicy = dpp; return this; }
        public Builder dataProtectionPolicy(DataProtectionPolicyPreset preset) {
            return dataProtectionPolicy( DataProtectionPolicy.fromPreset( preset ) );
        }
        public Builder dataProtectionPolicy(long commitLogRetention,
                                            long snapshotRetentionTime,
                                            RecurrenceRule rr,
                                            TimeUnit unit ) {
            List<SnapshotPolicy> sp = new ArrayList<>( );
            sp.add( new SnapshotPolicy( rr, Duration.of( snapshotRetentionTime, unit ) ) );
            return dataProtectionPolicy( new DataProtectionPolicy( Duration.of( commitLogRetention, unit), sp) );
        }

        public Volume create() {
            return null;
        }
    }

    private Tenant               tenant;
    private Map<String, String>  tags;
    private String               application;
    private VolumeStatus         status;
    private VolumeSettings       settings;
    private MediaPolicy          mediaPolicy;
    private DataProtectionPolicy dataProtectionPolicy;
    private VolumeAccessPolicy   accessPolicy;
    private QosPolicy            qosPolicy;

    Volume( ID uid,
            String name,
            Tenant tenant,
            String application,
            VolumeStatus status,
            VolumeSettings settings,
            MediaPolicy mediaPolicy,
            DataProtectionPolicy dataProtectionPolicy,
            VolumeAccessPolicy accessPolicy,
            QosPolicy qosPolicy,
            Map<String, String> tags ) {
        super( uid, tenant.getName(), name );
        this.tenant = tenant;
        this.tags = tags;
        this.application = application;
        this.status = status;
        this.settings = settings;
        this.mediaPolicy = mediaPolicy;
        this.dataProtectionPolicy = dataProtectionPolicy;
        this.accessPolicy = accessPolicy;
        this.qosPolicy = qosPolicy;
    }

    public Tenant getTenant() {
        return tenant;
    }

    public void setTenant( Tenant tenant ) {
        this.tenant = tenant;
    }

    public Map<String, String> getTags() {
        return tags;
    }

    public void addTag( String tag, String val ) {
        this.tags.put( tag, val );
    }

    public void removeTag( String tag ) {
        this.tags.remove( tag );
    }

    public String getApplication() {
        return application;
    }

    public void setApplication( String application ) {
        this.application = application;
    }

    public VolumeStatus getStatus() {
        return status;
    }

    public void setStatus( VolumeStatus status ) {
        this.status = status;
    }

    public VolumeSettings getSettings() {
        return settings;
    }

    public void setSettings( VolumeSettings settings ) {
        this.settings = settings;
    }

    public MediaPolicy getMediaPolicy() { return mediaPolicy; }

    public DataProtectionPolicy getDataProtectionPolicy() { return dataProtectionPolicy; }

    public QosPolicy getQosPolicy() { return qosPolicy; }

    public VolumeAccessPolicy getAccessPolicy() { return accessPolicy; }

}
