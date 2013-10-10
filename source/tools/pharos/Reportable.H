#ifndef _REPORTABLE_H_INCLUDED_
#define _REPORTABLE_H_INCLUDED_ 1
// Interface

class Reportable
{
public:
  Reportable() {}
  virtual ~Reportable() {}
  // Using compiler-generated versions for copy constructor and assignment
  // Reportable(Reportable const& old);
  // Reportable const& operator=(Reportable const& rhs);


};

#endif
