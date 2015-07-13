package com.formationds.iodriver.endpoints;

import com.formationds.iodriver.operations.BaseHttpOperation;

public interface BaseHttpEndpoint<ThisT extends BaseHttpEndpoint<ThisT, OperationT, ConnectionT>,
                                  OperationT extends BaseHttpOperation<OperationT,
                                                                       ? super ThisT,
                                                                       ? extends ConnectionT>,
                                  ConnectionT>
extends Endpoint<ThisT, OperationT>
{ }
