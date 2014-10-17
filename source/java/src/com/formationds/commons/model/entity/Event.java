/**
 * Copyright (c) 2014 Formation Data Systems. All rights reserved.
 */
package com.formationds.commons.model.entity;

import com.formationds.commons.model.abs.ModelBase;

import javax.persistence.*;
import javax.xml.bind.annotation.XmlRootElement;
import java.sql.Timestamp;
import java.time.Instant;

/**
 * TODO: Is an event an Entity to be persisted in the JPA DB like the VolumeDatapoint?
 *
 * Design note: this prototype departs from the common JPA pattern with public setters for everything.
 * Instead, it makes all of the setters protected, which still allows JPA to call the setters when loading the
 * object from the persistent store, but allows them to be treated as if they were immutable to clients.
 * It should also be possible to annotate fields and NOT have setters at all for those fields considered
 * immutable.
 */
@XmlRootElement
@Entity
public class Event extends ModelBase {

    /**
     * TODO: this may make more sense as subclass for each different type of event to allow different representations?
     */
    public enum EventType {
        USER_ACTIVITY,
        SYSTEM_EVENT
    }

    /**
     *
     */
    public enum EventCategory {
        PERFORMANCE,
        FIREBREAK,
        VOLUMES,
        STORAGE,
        SYSTEM
    }

    /**
     * TODO: do we want to represent state similar to Nagios
     * where there is a SOFT (detected problem, but give it some time to re-check)
     * and a HARD state type indicating configured re-check attempts have all failed.
     *
     * http://nagios.sourceforge.net/docs/nagioscore/4/en/statetypes.html
     */
    public enum EventState { SOFT, HARD, RECOVERED }

    /**
     * TODO: use SLF4J (or similar) severity definitions?
     */
    public enum Severity {
        /** Unknown severity */
        UNKNOWN,

        /** Represents a configuration change */
        CONFIG,

        /** The event is informational in nature.  No administrator action is required */
        INFO,

        /**
         * The event represents a warning that something in the system has occurred that
         * may require administrator action to identify and resolve.  The system is operating
         * normally at this time.
         */
        WARNING,

        /**
         * An error has occurred in the system and may require administrator action to
         * identify and resolve.  One or more features may not be operating properly, but
         * the system as a whole is operating.
         */
        ERROR,

        /**
         * A critical event indicates that one or more subsystems has an error that requires
         * immediate administrator action to identify and resolve.
         */
        CRITICAL,

        /**
         * A fatal event indicates that one or more subsystems has failed and the system is
         * not operating properly.  Recovery attempts have failed and immediate administrator
         * action is required.
         */
        FATAL
    }

    @Id
    @GeneratedValue(strategy = GenerationType.AUTO)
    private long id;

    @Enumerated(EnumType.ORDINAL) private EventType type;
    @Enumerated(EnumType.ORDINAL) private EventCategory category;
    @Enumerated(EnumType.ORDINAL) private Severity severity;
    @Enumerated(EnumType.ORDINAL) private EventState state;

    @Temporal(TemporalType.TIMESTAMP) private Timestamp initialTimestamp;
    @Temporal(TemporalType.TIMESTAMP)  private Timestamp modifiedTimestamp;

    // TODO: event details are different based on user activity vs system events.
    // At this point it's not clear if there is value in subclassing and breaking this
    // down further vs. stuffing all the details into a string message.  We'll start
    // here and enhance it as needed.

    /**
     * ResourceBundle message lookup key for the event.  Enables event message localization.
     */
    @Column(nullable = false)
    private String messageKey;

    /**
     * ResourceBundle message arguments for the event.
     */
    @Column(nullable = true)
    private Object[] messageArgs;

    /**
     * Default constructor for JPA support
     */
    protected Event() { }

    /**
     *
     * @param type
     * @param category
     * @param severity
     * @param messageKey resource bundle message lookup key
     * @param messageArgs resource bundle message arguments
     */
    public Event(EventType type, EventCategory category, Severity severity,
                 String messageKey, Object... messageArgs) {
        this.type = type;
        this.category = category;
        this.severity = severity;
        this.initialTimestamp = new Timestamp(Instant.now().toEpochMilli());
        this.messageKey = messageKey;
        this.messageArgs = (messageArgs != null ? messageArgs.clone() : new Object[0]);
        state = EventState.SOFT;
    }

    /**
     * Update the event state to the specified state.
     * <p/>
     * If the new state is identical to the current state, it is ignored.  If the
     * current state is not SOFT and the new state is SOFT, an IllegalArgumentException is thrown.
     * Otherwise, the event state is updated
     *
     * @param newState
     *
     * @throws java.lang.IllegalArgumentException if the state is not a valid transition
     */
    public void updateEventState(EventState newState) {
        if (state.equals(newState))
            return;

        switch (state) {
            case SOFT:
                // any other state is valid
                break;

            case HARD:
                if (!EventState.RECOVERED.equals(newState))
                    throw new IllegalArgumentException("Invalid state transition: " + state + "->" + newState);
                break;

            case RECOVERED:
                throw new IllegalArgumentException("Invalid state transition: " + state + "->" + newState);
        }

        this.state = newState;
        this.modifiedTimestamp = new Timestamp(Instant.now().toEpochMilli());
    }

    /**
     *
     * @return the generated event id.
     */
    public long getId() {
        return id;
    }

    /**
     * Note that this is provided to satisfy JPA persistence requirements.
     * The event id is a JPA generated value and should be treated as immutable
     *
     * @param id
     */
    protected void setId(long id) {
        this.id = id;
    }

    /**
     *
     * @return the event type
     */
    public EventType getType() {
        return type;
    }

    /**
     *
     * Note that this is provided to satisfy JPA persistence requirements.
     * The event type should be treated as immutable
     *
     * @param type
     */
    protected void setType(EventType type) {
        this.type = type;
    }

    /**
     *
     * @return the event category
     */
    public EventCategory getCategory() {
        return category;
    }

    /**
     * Set the event category.  Note that this is provided to satisfy JPA persistence requirements.
     * The category should be treated as immutable
     *
     * @param category
     */
    protected void setCategory(EventCategory category) {
        this.category = category;
    }

    /**
     *
     * @return the event severity
     */
    public Severity getSeverity() {
        return severity;
    }

    /**
     * Set the event severity.
     *
     * Note that this is provided to satisfy JPA persistence requirements.
     * The severity should be treated as immutable
     *
     * @param severity
     */
    protected void setSeverity(Severity severity) {
        this.severity = severity;
    }

    /**
     *
     * @return the event state
     */
    public EventState getState() {
        return state;
    }

    /**
     * Set the event state.
     *
     * Note that this is provided to satisfy JPA persistence requirements.  The state should
     * only be set through the {@link #updateEventState(com.formationds.commons.model.entity.Event.EventState)}
     * API.
     *
     * @param state
     */
    protected void setState(EventState state) {
        this.state = state;
    }

    /**
     *
     * @return the initial timestamp for this event
     */
    public Timestamp getInitialTimestamp() {
        return initialTimestamp;
    }

    /**
     * Set the initial timestamp
     *
     * Note that this is provided to satisfy JPA persistence requirements.
     * The initial timestamp should be treated as immutable
     *
     * @param initialTimestamp
     */
    protected void setInitialTimestamp(Timestamp initialTimestamp) {
        this.initialTimestamp = initialTimestamp;
    }

    /**
     * @return the last modified timestamp
     */
    public Timestamp getModifiedTimestamp() {
        return modifiedTimestamp;
    }

    /**
     * Set the last modified timestamp
     *
     * Note that this is provided to satisfy JPA persistence requirements.
     * The modified timestamp should only be modified internally through APIs that alter the event, such
     * as {@link #updateEventState(com.formationds.commons.model.entity.Event.EventState)}.
     *
     * @param modifiedTimestamp
     */
    protected void setModifiedTimestamp(Timestamp modifiedTimestamp) {
        this.modifiedTimestamp = modifiedTimestamp;
    }

    /**
     *
     * @return the message localization key
     */
    public String getMessageKey() {
        return messageKey;
    }

    /**
     * Set the message localization key
     *
     * Note that this is provided to satisfy JPA persistence requirements.
     * The message key should be treated as immutable

     * @param key
     */
    protected void setMessageKey(String key) {
        this.messageKey = key;
    }

    /**
     *
     * @return the message localization format args
     */
    public Object[] getMessageArgs() {
        return messageArgs;
    }

    /**
     * Note that this is provided to satisfy JPA persistence requirements.
     * The message args should be treated as immutable.
     *
     * @param args
     */
    protected void setMessageArgs(Object... args) {
        this.messageArgs = (args != null ? args.clone() : new Object[0]);
    }
}
