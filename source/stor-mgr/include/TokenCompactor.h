/*                                                                                                                                          
 * Copyright 2013 Formation Data Systems, Inc.                                                                                              
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_TOKENCOMPACTOR_H_
#define SOURCE_STOR_MGR_INCLUDE_TOKENCOMPACTOR_H_

#include <fds_types.h>
#include <ObjMeta.h>
#include <StorMgrVolumes.h>

/**
 * Current token compaction process:
 *
 * 1) startCompaction() method starts the compaction process. 
 * The compaction process first divides the range of object ids belonging
 * to this token into sub-ranges, and creates work items for each
 * sub-range and puts them to QoS queue
 *
 * 2) When work items are dispatched from QoS queue, each work item
 * is passed back to TokenCompactor for processing via prepareCopyItems()
 * method. For each work item includes object id subrange to process, 
 * we will get metadata from index db for all object ids belonging to this
 * subrange. For all existing object ids from this sub-range, token compactor
 * will create a list of objects and mark them either for deletion or copy.
 * Token compactor will divide this object list into sub-lists (one or more)
 * and create a 'copy' work item for each sub-list and put them to QoS queue.
 *
 * 3). When 'copy' work item is dispatched from qos queue, doObjCopy() method
 * is called. For each object in object list, we will verify with index db again
 * if each of this object must be copied or not, and copy if needed.
 * 
 * TokenCompactor keeps track which object ranges are compacted, and once all of
 * the objects of this token are compacted, TokenCompactor will call callback function
 * provided with startCompaction() method to notify that compaction is completed.
 */
#define GC_TOKEN_OID_RANGE_COUNT 2  // number of sub-ranges we will work on separately

namespace fds {

    /**
     * Callback type to notify that compaction process is finished for this token
     */
    typedef void (*compaction_done_handler_t)(fds_token_id token);

    /**
     * TokenCompactor is responsible for compacting storage for one token.
     * One can either create this class once for one particular token
     * and once compaction is done, delete this class object. Or can re-use
     * TokenCompactor object for compacting another token (but only when the
     * compaction state is TCSTATE_IDLE = not in the process of compacting another token)
     *
     * The compaction itself is driven by Scavenger class as follows:
     * start compaction for some token with startCompaction() passing event handler
     * for compaction done event.
     * Token compaction will notify when compaction is finished for a given token
     * by calling compaction done event handler.
     * Scavenger can ask about progress via getProgress(), e.g. if it wants to start
     * compaction process for more tokens when current tokens make enough progress, etc.
     */
    class TokenCompactor {
  public:
        TokenCompactor();
        ~TokenCompactor();

        typedef enum {
            TCSTATE_IDLE,         // state where we can start token compaction
            TCSTATE_IN_PROGRESS,  // compaction is in progress
            TCSTATE_ERROR         // error happened, in recovery (hopefully)
        } tcStateType;

        /**
         * Tells tokenFileDb that we start GC for this token, which will
         * create a shadow token file to which new puts will go.
         * Creates work items items for object id ranges (work itemt to
         * iterate index db and decide which objects to copy) and put them
         * to QoS queue.
         * Once this function is called, there is nothing else to do for the
         * scavenger class. Scavenger will receive a callback when GC is finished
         * for this token id. Scavenger can also check on progress of GC via
         * getProgress() method.
         * @return ERR_OK is success; error if current state is not TCSTATE_IDLE
         **/
        Error startCompaction(fds_token_id token_id,
                              compaction_done_handler_t done_evt_hdlr);


        /**
         * returns number from 0 to 100 (in percent, approximate) how many
         * objects got copied
         */
        fds_uint32_t getProgress() const;

        /**
         * For a given object id range, iterates through index DB and decides
         * which objects to copy (and which are deleted) and prepares one or
         * more object lists for object copy
         */
        Error prepareCopyItems();

        /**
         * Copy non-deleted objects from a given object list to new file
         */
        Error doObjCopy();

        /**
         * Given object metadata, check if this object should be garbage
         * collected.
         * @return true if object should be garbage collected, otherwise false
         */
        static fds_bool_t isGarbage(const ObjMetaData& md);

  private:  // methods
        /**
         * Tells tokenFileDB that GC for the token is finished, sets the compactor
         * state to idle and calls callback function provided in startCompaction()
         * to notify that GC for this tone is finished.
         */
        Error handleCompactionDone();

  private:  // members
        /**
         * Token compactor state
         */
        tcStateType state;

        /**
         * Token id the compactor currently operates on. If state is TCSTATE_IDLE,
         * then token_id is not used, and undefined
         */
        fds_token_id token_id;  // token id this compactor currently operates on

        /**
         * callback for compaction done method which is set every time
         * startCompaction() is called. When state is TCSTATE_IDLE, this cb
         * is undefined.
         */
        compaction_done_handler_t done_evt_hdlr;  // cb for compaction done
    };

    typedef boost::shared_ptr<TokenCompactor> TokenCompactorPtr;

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_TOKENCOMPACTOR_H_
