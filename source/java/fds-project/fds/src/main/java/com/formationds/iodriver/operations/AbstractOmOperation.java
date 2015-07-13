package com.formationds.iodriver.operations;

import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;

/**
 * An operation that runs on the OM.
 */
public abstract class AbstractOmOperation
extends AbstractHttpsOperation<AbstractOmOperation, OrchestrationManagerEndpoint>
implements OmOperation
{ }
