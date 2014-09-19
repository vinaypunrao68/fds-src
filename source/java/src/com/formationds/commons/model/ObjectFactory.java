/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
 *
 *  This software is furnished under a license and may be used and copied only
 *  in  accordance  with  the  terms  of such  license and with the inclusion
 *  of the above copyright notice. This software or  any  other copies thereof
 *  may not be provided or otherwise made available to any other person.
 *  No title to and ownership of  the  software  is  hereby transferred.
 *
 *  The information in this software is subject to change without  notice
 *  and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 *  Formation Data Systems assumes no responsibility for the use or  reliability
 *  of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.commons.model;

/**
 * @author ptinius
 */
public class ObjectFactory
{
  /**
   * @return Returns {@link Connector}
   */
  public static Connector createConnector()
  {
    return new Connector();
  }

  /**
   * @return Returns {@link Usage}
   */
  public static Usage createCurrentUsage()
  {
    return new Usage();
  }

  /**
   * @return Returns {@link Domain}
   */
  public static Domain createDomain()
  {
    return new Domain();
  }

  /**
   * @return Returns {@link Node}
   */
  public static Node createNode()
  {
    return new Node();
  }

  /**
   * @return Returns {@link Service}
   */
  public static Service createService()
  {
    return new Service();
  }

  /**
   * @return Returns {@link Snapshot}
   */
  public static Snapshot createSnapshot()
  {
    return new Snapshot();
  }

  /**
   * @return Returns {@link SnapshotPolicy}
   */
  public static SnapshotPolicy createSnapshotPolicy()
  {
    return new SnapshotPolicy();
  }

  /**
   * @return Returns {@link Stream}
   */
  public static Stream createStream()
  {
    return new Stream();
  }

  /**
   * @return Returns {@link Tenant}
   */
  public static Tenant createTenant()
  {
    return new Tenant();
  }

  /**
   * @return Returns {@link User}
   */
  public static User createUser()
  {
    return new User();
  }

  /**
   * @return Returns {@link Volume}
   */
  public static Volume createVolume()
  {
    return new Volume();
  }

  /**
   * @return Returns {@link Status}
   */
  public static Status createStatus()
  {
    return new Status();
  }

  /**
   * @return Returns {@link Metadata}
   */
  public static Metadata createMetadata() { return new Metadata(); }
}
