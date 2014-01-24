#include <iostream>
#include <atomic>
#include <vector>
#include <boost/noncopyable.hpp>
#include <fds_assert.h>

/* Forward declarations */
class FdsCounters;
class FdsBaseCounter;
class FdsCountersMgr;

/**
 * @brief Counter manager.  Mananges the job of exporting registered
 * FdsCounters class objects.
 */
class FdsCountersMgr : public boost::noncopyable {
 public:
  void add_for_export(FdsCounters *counters);
  void remove_from_export(FdsCounters *counters);

  std::string export_as_graphite();
 protected:
  std::vector<FdsCounters*> exp_counters;
};

/**
 * @brief Base counters class
 */
class FdsCounters : public boost::noncopyable { 
 public:
  FdsCounters(const std::string &id, FdsCountersMgr *mgr)
      : id_(id)
  {
      if (mgr) {
          mgr->add_for_export(this);
      }
  }

 protected:
  /**
   * @brief Marks the counter for export.  Only export counters
   * that are members of the derived class.  This method is invoked
   * by FdsBaseCounter constructor to mark the counter as exported.
   *
   * @param cp Counter to export
   */
  void add_for_export(FdsBaseCounter* cp)
  {
      fds_assert(cp);
      exp_counters_.push_back(cp);
  }

  void remove_from_export(FdsBaseCounter* cp)
  {
      fds_verify(!"Not implemented yet");
  }

 protected:
  /* Exported counters */
  std::vector<FdsBaseCounter*> exp_counters_;
  /* Id of this counter set */
  std::string id_;
  
  friend class FdsBaseCounter;
};

/**
 * @brief Base class for counters that are to be exported.
 */
class FdsBaseCounter : public boost::noncopyable {
 public:
  /**
   * @brief  Base counter constructor.  Enables a counter to
   * be exported with an identifier.  If export_parent is NULL
   * counter will not be exported.
   *
   * @param id - id to use when exporting the counter
   * @param export_parent - Pointer to the parent.  If null counter
   * is not exported.
   */
  FdsBaseCounter(const std::string &id, FdsCounters *export_parent)
      : id_(id)
  {
      if (export_parent) {
          export_parent->add_for_export(this);
      }
  }

  virtual uint64_t value() const = 0;
  virtual std::string id() const
  {
      return id_;
  }

 private:
  std::string id_;
};

class NumericCounter : public FdsBaseCounter, std::atomic<uint64_t> 
{
 public:
  NumericCounter(const std::string &id, FdsCounters *export_parent)
      : FdsBaseCounter(id, export_parent)
  {
  }

  virtual uint64_t value() const override
  {
      return load();
  }
};




/**
 * @brief 
 *
 * @param id
 * @param counters
 */
void FdsCountersMgr::add_for_export(FdsCounters *counters)
{
}

/**
 * @brief 
 *
 * @param counters
 */
void FdsCountersMgr::remove_from_export(FdsCounters *counters)
{
}

std::string FdsCountersMgr::export_as_graphite()
{
    return "";
}

class SMCounters : public FdsCounters
{
 public:
  SMCounters(const std::string &id, FdsCountersMgr *mgr)
      : FdsCounters(id, mgr),
        puts_cntr("puts_cntr", this),
        gets_cntr("gets_cntr", this)
  {
  }

  NumericCounter puts_cntr;
  NumericCounter gets_cntr;

};

int main() {
    return 0;
}
