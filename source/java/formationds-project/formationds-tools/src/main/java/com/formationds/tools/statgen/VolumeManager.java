/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.tools.statgen;

import com.formationds.client.v08.model.Size;
import com.formationds.client.v08.model.SizeUnit;
import com.formationds.client.v08.model.Tenant;
import com.formationds.client.v08.model.Volume;
import com.formationds.client.v08.model.VolumeSettings;

import java.util.List;

public interface VolumeManager {
    void createBlockVolume( String domain,
                            String name,
                            Tenant tenant,
                            long capacity,
                            long blockSize,
                            SizeUnit unit ) throws RestClient.ClientException;

    void createBlockVolume( String domain,
                            String name,
                            Tenant tenant,
                            Size capacity,
                            Size blockSize ) throws RestClient.ClientException;

    void createObjectVolume( String domain,
                             String name,
                             Tenant tenant,
                             long maxObjectSize,
                             SizeUnit unit ) throws RestClient.ClientException;

    void createObjectVolume( String domain,
                             String name,
                             Tenant tenant,
                             Size maxObjectSize ) throws RestClient.ClientException;

    void createVolume( String domain,
                       String name,
                       Tenant tenant,
                       VolumeSettings settings ) throws RestClient.ClientException;

    void createVolume( Volume volume ) throws RestClient.ClientException;

    List<Volume> listVolumes() throws RestClient.ClientException;

    List<Volume> listVolumes( String notUsedDomainName ) throws RestClient.ClientException;

    long getVolumeId( String notUsedDomainName,
                      String volumeName ) throws RestClient.ClientException;

    void deleteVolume( String domainName, String volumeName )
        throws RestClient.ClientException;

    Volume statVolume( String domainName, String volumeName )
        throws RestClient.ClientException;
}
