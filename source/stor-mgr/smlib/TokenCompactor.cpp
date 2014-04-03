/*                                                                                                                                          
 * Copyright 2014 Formation Data Systems, Inc.                                                                                              
 */

#include <TokenCompactor.h>

namespace fds {

TokenCompactor::TokenCompactor()
        : state(TCSTATE_IDLE),
          token_id(0),
          done_evt_hdlr(NULL)
{
}

TokenCompactor::~TokenCompactor()
{
}

//
// Creates a new file for the token in tokenFileDb to which non-deleted
// objects will be copied and new puts will go.
// Decides on object id ranges to compact at a time, and creates
// work items to actually get list of objects that need copy /delete
//
Error TokenCompactor::startCompaction(fds_token_id token_id,
                                      compaction_done_handler_t done_evt_hdlr)
{
    Error err(ERR_OK);

    LOGNORMAL << "Start Compaction of token " << token_id;

    // check token state if it's already in the middle of compaction

    // first tell tokenFileDB to create new file and start re-routing
    // IO to the correct file

    // create filter range work items and put them to qos queue

    return err;
}


//
// Prepares work items for a given object id range to actually copy objects to
// new tokenFileDB; could be one or more work items
//
Error TokenCompactor::prepareCopyItems()
{
    Error err(ERR_OK);

    return err;
}

//
// Do actualy copy for given objects
//
Error TokenCompactor::doObjCopy()
{
    Error err(ERR_OK);

    return err;
}

Error TokenCompactor::handleCompactionDone()
{
    Error err(ERR_OK);

    return err;
}

//
// returns number from 0 to 100 (percent of progress, approx)
//
fds_uint32_t TokenCompactor::getProgress() const
{
    return 0;
}

//
// given object metadata, check if we need to garbage collect it
//
fds_bool_t TokenCompactor::isGarbage(const ObjMetaData& md)
{
    fds_bool_t do_gc = false;

    // TODO(anna or Vinay) add other policies for checking GC
    // The first version of this method just decides based on
    // refcount -- if < 1 then garbage collect
    if (md.getRefCnt() < 1) {
        do_gc = true;
    }

    return do_gc;
}
}  // namespace fds
