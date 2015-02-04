package com.formationds.commons.patterns;

// @eclipseFormat:off
public interface VisitableWithArg<
        VisitorT extends VisitorWithArg<ThisT, VisitorT, ArgumentT, ExT>,
        ThisT extends VisitableWithArg<VisitorT, ThisT, ArgumentT, ExT>,
        ArgumentT,
        ExT extends Exception>
{
    void accept(VisitorT visitor, ArgumentT argument) throws ExT;
}
