package com.formationds.om.events;

import com.formationds.commons.events.EventCategory;
import com.formationds.commons.events.EventDescriptor;
import com.formationds.commons.events.EventSeverity;
import com.formationds.commons.events.EventType;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public enum OmEvents implements EventDescriptor {

    CREATE_LOCAL_DOMAIN(EventCategory.SYSTEM, "Created local domain {0}", "domainName", "domainSite"),
    CREATE_TENANT(EventCategory.VOLUMES, "Created tenant {0}", "identifier"),
    CREATE_USER(EventCategory.SYSTEM, "Created user {0} - admin={1}; id={2}", "identifier", "isFdsAdmin", "userId"),
    UPDATE_USER(EventCategory.SYSTEM, "Updated user {0} - admin={1}; id={2}", "identifier", "isFdsAdmin", "userId"),
    ASSIGN_USER_TENANT(EventCategory.SYSTEM, "Assigned user {0} to tenant {1}", "userId", "tenantId"),
    REVOKE_USER_TENANT(EventCategory.SYSTEM, "Revoked user {0} from tenant {1}", "userId", "tenantId"),
    CREATE_QOS_POLICY(EventCategory.VOLUMES, "Created QoS policy {0}", "policyName"),
    MODIFY_QOS_POLICY(EventCategory.VOLUMES, "Modified QoS policy {0}", "policyName"),
    DELETE_QOS_POLICY(EventCategory.VOLUMES, "Deleted QoS policy {0}", "policyName"),
    CREATE_VOLUME(EventCategory.VOLUMES, "Created volume: domain={0}; name={1}; tenantId={2}; type={3}; size={4}",
                  "domainName", "volumeName", "tenantId", "volumeType", "maxSize"),
    DELETE_VOLUME(EventCategory.VOLUMES, "Deleted volume: domain={0}; name={1}", "domainName", "volumeName"),
    CREATE_SNAPSHOT_POLICY(EventCategory.VOLUMES, "Created snapshot policy {0} {1} {2}; id={3}",
                           "name", "recurrence", "retention", "id"),
    DELETE_SNAPSHOT_POLICY(EventCategory.VOLUMES, "Deleted snapshot policy: id={0}", "id"),
    DELETE_SNAPSHOT(EventCategory.VOLUMES, "Deleted snapshot: volume id={0} snapshot id={0}", "volumeId", "snapshotId"),
    ATTACH_SNAPSHOT_POLICY(EventCategory.VOLUMES, "Attached snapshot policy {0} to volume id {1}",
                           "policyId", "volumeId"),
    DETACH_SNAPSHOT_POLICY(EventCategory.VOLUMES, "Detached snapshot policy {0} from volume id {1}",
                           "policyId", "volumeId"),
    CREATE_SNAPSHOT(EventCategory.VOLUMES, "Created snapshot {0} of volume {1}.  Retention time = {2}",
                    "snapshotName", "volumeId", "retentionTime"),

    CLONE_VOLUME(EventCategory.VOLUMES,
                 "Cloned volume {0} with policy id {1}; clone name={2}; Cloned volume Id = {3}",
                 "volumeId", "policyId", "clonedVolumeName", "clonedVolumeId"),
    RESTORE_CLONE(EventCategory.VOLUMES, "Restored cloned volume {0} with snapshot id {1}", "volumeId",
                  "snapshotId"),
    
    /** Nodes Stuff **/
	ADD_NODE(
			EventType.SYSTEM_EVENT, EventCategory.SYSTEM, EventSeverity.ERROR, 
			"Added node {0} successfully", "nodeUuid"), 
	ADD_NODE_ERROR(
			EventType.SYSTEM_EVENT, EventCategory.SYSTEM, EventSeverity.ERROR,
			"Adding node uuid {0} failed.", "nodeUuid"),
	REMOVE_NODE(
			EventCategory.SYSTEM, "Removed node {0}:{1} successfully", "nodeName",
			"nodeUuid"), 
	REMOVE_NODE_ERROR(EventType.SYSTEM_EVENT,
			EventCategory.SYSTEM, EventSeverity.ERROR,
			"Removing node {0}:{1} failed.", "nodeName", "nodeUuid"),	
    START_NODE(EventType.SYSTEM_EVENT, EventCategory.SYSTEM, EventSeverity.INFO ,
    		"Node {0} was started.", "nodeUuid"),
    STOP_NODE(EventType.SYSTEM_EVENT, EventCategory.SYSTEM, EventSeverity.INFO ,
    		"Node {0} was stopped.", "nodeUuid"),
    CHANGE_NODE_STATE_FAILED(EventType.SYSTEM_EVENT ,EventCategory.SYSTEM, EventSeverity.ERROR,
    		"The attempt to change node state for node: {0} failed.", "nodeUuid"),
    CHANGE_SERVICE_STATE(EventType.SYSTEM_EVENT, EventCategory.SYSTEM, EventSeverity.INFO,
    		"Service: {0} state was changed successfully.", "serviceId" ),
    CHANGE_SERVICE_STATE_ERROR(EventType.SYSTEM_EVENT, EventCategory.SYSTEM, EventSeverity.INFO,
    	    "The attempt to change the service state for service: {0} failed.", "serviceId" ),
    REMOVE_SERVICE(EventType.SYSTEM_EVENT, EventCategory.SYSTEM, EventSeverity.INFO,
    		"Service {0} was removed from the system.", "serviceId"),
    REMOVE_SERVICE_ERROR(EventType.SYSTEM_EVENT, EventCategory.SYSTEM, EventSeverity.ERROR,
    		"Removal of service {0} failed.", "serviceId"),
    ADD_SERVICE(EventType.SYSTEM_EVENT, EventCategory.SYSTEM, EventSeverity.INFO,
    		"Service {0} was added to the system.", "serviceId"),
    ADD_SERVICE_ERROR(EventType.SYSTEM_EVENT, EventCategory.SYSTEM, EventSeverity.ERROR,
    		"Adding of service {0} failed.", "serviceId"),
    STOP_SERVICE(EventType.SYSTEM_EVENT, EventCategory.SYSTEM, EventSeverity.INFO,
    		"Service {0} was stopped in the system.", "serviceId"),
    STOP_SERVICE_ERROR(EventType.SYSTEM_EVENT, EventCategory.SYSTEM, EventSeverity.ERROR,
    		"Stopping of service {0} failed.", "serviceId"),
    START_SERVICE(EventType.SYSTEM_EVENT, EventCategory.SYSTEM, EventSeverity.INFO,
    		"Service {0} was started in the system.", "serviceId"),
    START_SERVICE_ERROR(EventType.SYSTEM_EVENT, EventCategory.SYSTEM, EventSeverity.ERROR,
    		"Starting of service {0} failed.", "serviceId");

    private final EventType     type;
    private final EventCategory category;
    private final EventSeverity severity;
    private final String        defaultMessage;
    private final List<String>  argNames;

    private OmEvents(EventCategory category, String defaultMessage, String... argNames) {
        this(EventType.USER_ACTIVITY, category, EventSeverity.CONFIG, defaultMessage, argNames);
    }

    private OmEvents(EventType type, EventCategory category, EventSeverity severity,
                        String defaultMessage, String... argNames) {
        this.type = type;
        this.category = category;
        this.severity = severity;
        this.defaultMessage = defaultMessage;
        this.argNames = (argNames != null ? Arrays.asList(argNames) : new ArrayList<String>(0));
    }

    public String key() { return name(); }

    public List<String> argNames() { return argNames; }

    public EventType type() { return type; }

    public EventCategory category() { return category; }

    public EventSeverity severity() { return severity; }

    public String defaultMessage() { return defaultMessage; }

}