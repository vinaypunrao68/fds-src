/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.util.thrift.svc;

import com.formationds.commons.model.Domain;
import com.formationds.commons.model.Node;
import com.formationds.protocol.svc.types.DomainNodes;

import java.util.List;
import java.util.Map;

/**
 * @author ptinius
 */
public abstract interface SvcLayerClientIface
{
    default public Map<Domain, List<Node>> getDomainNodes()
        throws SvcLayerException {

        return getDomainNodes(null);
    }

    public Map<Domain, List<Node>> getDomainNodes(
        final DomainNodes domainNodes )
        throws SvcLayerException;
}
