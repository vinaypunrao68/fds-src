/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
 *
 * This software is furnished under a license and may be used and copied only
 * in  accordance  with  the  terms  of such  license and with the inclusion
 * of the above copyright notice. This software or  any  other copies thereof
 * may not be provided or otherwise made available to any other person.
 * No title to and ownership of  the  software  is  hereby transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 * Formation Data Systems assumes no responsibility for the use or  reliability
 * of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.om.rest.snapshot;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;

import java.util.Map;

/**
 * @author ptinius
 */
public class CreateSnapshotPolicy
  implements RequestHandler
{
  private static final Logger LOG = Logger.getLogger(CreateSnapshotPolicy.class);

  private final Xdi xdi;
  private final FDSP_ConfigPathReq.Iface legacyConfigPath;
  private final AuthenticationToken token;

  public CreateSnapshotPolicy( final Xdi xdi,
                               final FDSP_ConfigPathReq.Iface legacyConfigPath,
                               final AuthenticationToken token )
  {
    super();
    this.xdi = xdi;
    this.legacyConfigPath = legacyConfigPath;
    this.token = token;
  }

  @Override
  public Resource handle( final Request request,
                          final Map<String, String> routeParameters )
    throws Exception
  {
    // TODO finish implementation
    return null;
  }
}
