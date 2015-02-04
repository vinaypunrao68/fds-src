package com.formationds.commons.patterns;

// @eclipseFormat:off
public interface VisitorWithArg<
        VisitableT extends VisitableWithArg<ThisT, VisitableT, ArgumentT, ExT>,
        ThisT extends VisitorWithArg<VisitableT, ThisT, ArgumentT, ExT>,
        ArgumentT,
        ExT extends Exception>
// @eclipseFormat:on
{
    void doVisit(VisitableT visitable, ArgumentT argument) throws ExT;
}
