package com.formationds.iodriver.operations;

import com.formationds.commons.patterns.VisitableWithArg;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.reporters.WorkflowEventListener;

// @eclipseFormat:off
public abstract class Operation<ThisT extends Operation<ThisT, EndpointT>,
                                EndpointT extends Endpoint<EndpointT, ThisT>>
implements VisitableWithArg<EndpointT, ThisT, WorkflowEventListener, ExecutionException>
//@eclipseFormat:on
{}
