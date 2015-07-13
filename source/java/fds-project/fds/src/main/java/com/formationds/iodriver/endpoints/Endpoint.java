package com.formationds.iodriver.endpoints;

import com.formationds.commons.patterns.VisitorWithArg;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public interface Endpoint<ThisT extends Endpoint<ThisT, OperationT>,
                          OperationT extends Operation<OperationT, ? super ThisT>>
extends VisitorWithArg<OperationT, ThisT, AbstractWorkflowEventListener, ExecutionException>
{ }
