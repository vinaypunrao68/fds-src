/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

/**
 * Represent the set of Volume states
 */
// maps to common.thrift ResourceState
public enum VolumeState {
    Unknown,

    /** resource is loading or in the middle of creation */
    Loading,

    /** resource has been created */
    Created,

    /** resource activated - ready to use */
    Active,

    /** resource is offline - expected to come back later */
    Offline,

    /** The system has received a request to delete the resource. */
    MarkedForDeletion,

    /** resource is gone now and will not come back */
    Deleted,

    /** in known erroneous state */
    InError,
}
