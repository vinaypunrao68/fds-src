// This is a very small shell class.  It collects the globals that
// describe one experiment.  For now, it is just the name of the
// experiment.

#ifndef _PHAROS_Experiment_H_INCLUDED_
#define _PHAROS_Experiment_H_INCLUDED_ 1

class Experiment
{
public:
  // If you want an Experiment object, this function will give it to you.
  // If nobody has initialized the experiment, this function will assert.
  friend Experiment* experiment();

  // If you want to create and initialize the global experiment, call
  // this function instead:
  // TODO: Replace char * with a string object that is copied.
  friend Experiment* experiment(char const* name);

  // Protected constructor, to make sure it stays a singleton:
protected:
  Experiment(char const* name_in);

  virtual ~Experiment()
  {}

public:
  // If the experiment has a name, returns it.  Warning: Might return
  // an empty string.
  char const* getName() const;

  // Returns if an experiment name with non-zero length is present:
  bool isNamePresent() const;

private:
  char const* name;

  static Experiment* it;

};

// See comments above
Experiment* experiment();
Experiment* experiment(char const* name);

#endif // _PHAROS_Experiment_H_INCLUDED_

