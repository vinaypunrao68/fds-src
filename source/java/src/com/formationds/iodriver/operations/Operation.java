package com.formationds.iodriver.operations;

import com.formationds.commons.patterns.VisitableWithArg;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.reporters.WorkflowEventListener;

/**
 * An operation that may be executed.
 * 
 * @param <ThisT> The implementing class.
 * @param <EndpointT> The type of endpoint this operation can run on.
 */
// @eclipseFormat:off
public abstract class Operation<ThisT extends Operation<ThisT, EndpointT>,
                                EndpointT extends Endpoint<EndpointT, ThisT>>
implements VisitableWithArg<EndpointT, ThisT, WorkflowEventListener, ExecutionException>
//@eclipseFormat:on
{}
