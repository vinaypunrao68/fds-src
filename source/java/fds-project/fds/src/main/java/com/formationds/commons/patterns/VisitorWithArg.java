package com.formationds.commons.patterns;

/**
 * A visitor as in the Visitor pattern that also provides arguments when visiting.
 * 
 * @param <VisitableT> The type of objects that may be visited.
 * @param <ThisT> The implementing class.
 * @param <ArgumentT> The type of argument when visiting.
 * @param <ExT> The type of exception that may be thrown when visiting.
 */
// @eclipseFormat:off
public interface VisitorWithArg<
        VisitableT extends VisitableWithArg<? super ThisT, VisitableT, ArgumentT, ? extends ExT>,
        ThisT extends VisitorWithArg<VisitableT, ThisT, ArgumentT, ExT>,
        ArgumentT,
        ExT extends Exception>
// @eclipseFormat:on
{
    /**
     * Visit an object.
     * 
     * @param visitable The object to visit.
     * @param argument The argument to provide when visiting.
     * 
     * @throws ExT when necessary.
     */
    void doVisit(VisitableT visitable, ArgumentT argument) throws ExT;
}
