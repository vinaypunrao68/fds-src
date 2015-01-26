package com.formationds.iodriver.operations;

import java.util.stream.Stream;

public interface OperationSource
{
    Stream<Operation> getOperations();
}
