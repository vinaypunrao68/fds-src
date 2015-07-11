package com.formationds.iodriver.operations;

import com.formationds.commons.patterns.VisitableWithArg;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public interface Operation<ThisT extends Operation<ThisT, EndpointT>,
                           EndpointT extends Endpoint<EndpointT, ? super ThisT>>
extends VisitableWithArg<EndpointT, ThisT, AbstractWorkflowEventListener, ExecutionException>
{ }
