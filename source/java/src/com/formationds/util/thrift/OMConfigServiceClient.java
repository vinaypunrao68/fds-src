/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */
package com.formationds.util.thrift;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeSettings;
import com.formationds.security.AuthenticationToken;

import java.util.List;

/**
 * Client interface to the OM configuration API.
 * <p/>
 * This interface is currently designed to mirror the limited portion of
 * the OM ConfigurationService.Iface API that is called by the XDI connector
 * implementations such as s3 and swift.
 * <p/>
 * The default implementation of this interface is a REST client to the OM that
 * interacts with the OM via it's REST API.  @see ConfigurationServiceRestImpl
 */
public interface OMConfigServiceClient {

    /**
     *
     * @param token
     * @param domain
     * @param name
     * @param volumeSettings
     * @param tenantId
     * @throws OMConfigException
     */
    void createVolume(AuthenticationToken token,
                      String domain,
                      String name,
                      VolumeSettings volumeSettings,
                      long tenantId) throws OMConfigException;

    /**
     *
     * @param token
     * @param domainName
     * @return the list of volumes
     * @throws OMConfigException on authentication or communication error
     */
    List<VolumeDescriptor> listVolumes(AuthenticationToken token, String domainName) throws OMConfigException;

    /**
     *
     * @param token
     * @param domainName
     * @param volumeName
     *
     * @return the volume id if it exists, 0 if the volume does not exist.
     *
     * @throws OMConfigException if authentication fails or a communication error occurs.
     */
    long getVolumeId(AuthenticationToken token, String domainName, String volumeName) throws OMConfigException;

    /**
     * @param token
     * @param domainName
     * @param volumeName
     * @throws OMConfigException
     */
    void deleteVolume(AuthenticationToken token, String domainName, String volumeName) throws OMConfigException;

    /**
     * @param token
     * @param domainName
     * @param volumeName
     * @return the VolumeDescriptor for the requested volume; null of a non-existent volume
     * @throws OMConfigException if authentication fails or a communication error occurs.
     */
    VolumeDescriptor statVolume(AuthenticationToken token, String domainName, String volumeName) throws OMConfigException;
}
