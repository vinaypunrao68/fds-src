/*                                                                                                                                          
 * Copyright 2013 Formation Data Systems, Inc.                                                                                              
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_TOKENCOMPACTOR_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_TOKENCOMPACTOR_H_

#include <list>
#include <vector>
#include <fds_types.h>
#include <fds_timer.h>
#include <ObjMeta.h>
#include <SmIo.h>

/**
 * Current token compaction process:
 *
 * 1) startCompaction() method starts the compaction process. It tells
 * the persistence layer to start writing new data to new file, but still
 * keep the old file (for reads while we are compacting data). It starts
 * timer to go to next step.
 *
 * 2) Send an FDS_SM_SNAPSHOT_TOKEN request to QoS queue. When this request
 * is processed we take a snapshot of index db and do step 3.
 *
 * 2) We iterate through the snapshot of index db and create a work item
 * (an SM IO request of type FDS_SM_COMPACT_OBJECTS) per each
 * GC_COPY_WORKLIST_SIZE number of object ids. Before we create work items,
 * we sort all object ids by their offsets, so that when we copy objects,
 * we are not seeking all over the place in the token file.
 *
 * 3). When 'copy' work item is dispatched from qos queue, compactObjectsCb() method
 * is called. For each object in object list, we will verify with index db again
 * if each of this object must be copied or not, and copy if needed.
 * 
 * TokenCompactor keeps track of objects it compacted, and once all of 
 * the objects of this token are compacted, TokenCompactor will call callback function
 * provided with startCompaction() method to notify that compaction is completed.
 */

#define GC_COPY_WORKLIST_SIZE 10

namespace fds {

class SmPersistStoreHandler;

    /**
     * Callback type to notify that compaction process is finished for this token
     */
    typedef std::function<void (fds_token_id token,
                                const Error& error)> compaction_done_handler_t;

    /**
     * Callback type to continue iterating over snapshot
     */
    typedef std::function<void()> ContinueWorkFn;

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
        TokenCompactor(SmIoReqHandler *_data_store,
                       SmPersistStoreHandler* persist_store);
        ~TokenCompactor();

        typedef enum {
            TCSTATE_IDLE = 0,      // state where we can start token compaction
            TCSTATE_PREPARE_WORK,  // preparing compaction work
            TCSTATE_IN_PROGRESS,   // compaction is in progress
            TCSTATE_DONE,          // we are done but cleaning up temp state
            TCSTATE_ERROR          // error happened, in recovery (hopefully)
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
         * Method is thread-safe
         *
         * @param num_bits_for_token get be retreived from DLT::getNumBitsForToken()
         * @return ERR_OK is success; error if current state is not TCSTATE_IDLE
         **/
        Error startCompaction(fds_token_id tok_id,
                              fds_uint16_t disk_id,
                              diskio::DataTier tier,
                              fds_bool_t verify,
                              compaction_done_handler_t done_evt_hdlr);


        /**
         * returns number from 0 to 100 (in percent, approximate) how many
         * objects got copied
         */
        fds_uint32_t getProgress() const;

        /**
         * @return true if compactor is idle = can start new compaction job
         */
        fds_bool_t isIdle() const;

        /**
         * Given object metadata, check if this object should be garbage
         * collected from storage media.
         * @return true if object should be garbage collected, otherwise false
         */
        static fds_bool_t isDataGarbage(const ObjMetaData& md,
                                        diskio::DataTier _tier);


        /**
         * Given object metadata, check if this object should be garbage
         * collected from both storage media and index db
         * @return true if object should be garbage collected from index db and
         * storage media, otherwise false
         */
        static fds_bool_t isGarbage(const ObjMetaData& md);

        /**
         * Callback from object store that metadata snapshot is complete
         */
        void snapDoneCb(const Error& error,
                        SmIoSnapshotObjectDB* snap_req,
                        leveldb::ReadOptions& options,
                        std::shared_ptr<leveldb::DB> db);

        /**
         * Perform the work of iterating over the leveldb and sending out requests for objects to be compacted
         * this should provide some level of rate limiting such that we can't have too many outstanding IO requests
         * at the same time.
         */
         void compactionWorker(leveldb::Iterator *it,
                               std::shared_ptr<leveldb::DB> db,
                               leveldb::ReadOptions& options,
                               bool last_run);

        /**
         * Callback from object store that compaction for a set of objects is
         * finished
         */
        void objsCompactedCb(const Error& error,
                             SmIoCompactObjects* req,
                             ContinueWorkFn nextWork);


        void handleTimerEvent();

  private:  // methods
        /**
         * Enqueue command to take indexDB snapshot
         */
        void enqSnapDbWork();
        /**
         * Enqueue objects copy request to QoS queue
         * @param obj_list list of object ids to work on, when method
         * returns this list will be empty
         */
        Error enqCopyWork(std::vector<ObjectID>* obj_list, ContinueWorkFn nextWork);
        /**
         * Tells tokenFileDB that GC for the token is finished, sets the compactor
         * state to idle and calls callback function provided in startCompaction()
         * to notify that GC for this tone is finished.
         */
        Error handleCompactionDone(const Error& tc_error);

  private:  // members
        /**
         * Token compactor state
         */
        std::atomic<tcStateType> state;

        /**
         * Token id the compactor currently operates on. If state is TCSTATE_IDLE,
         * then token_id is not used, and undefined
         */
        fds_token_id token_id;
        fds_uint16_t cur_disk_id;   // disk id the compactor currently operates on
        diskio::DataTier cur_tier;  // tier we are compacting
        // TODO(Anna) we are piggybacking data verification on the same process as
        // token compaction;
        // We should consider making this class a base class for background tasks
        fds_bool_t verifyData;

        /**
         * callback for compaction done method which is set every time
         * startCompaction() is called. When state is TCSTATE_IDLE, this cb
         * is undefined.
         */
        compaction_done_handler_t done_evt_handler;

        /**
         * pointer to SmIoReqHandler so we can queue work to QoS queues
         * passed in constructor, does not own
         */
        SmIoReqHandler *data_store;
        /**
         * Pointer to SmPersistStoreHandler for communicating with
         * persistent data store for GC related tasks
         */
        SmPersistStoreHandler *persistGcHandler;
        /**
         * request to snapshot index db for a given token, we re-use this object
         * but we set appropriate fiels based on current compaction request
         */
        SmIoSnapshotObjectDB snap_req;

        /**
         * For tracking compaction progress
         */
        fds_uint32_t total_objs;  // total number of objs we will either copy or del
        std::atomic<fds_uint32_t> objs_done;  // current number of objs we processed

        /**
         * We use this timer in two stages of compaction:
         * 1) Timer to enqueue snap DB work after we start writing to new token file
         * so that all IOs that are currently in flight (writing to old file before
         * updating index db) finish and so that we do not miss objects that are in
         * old file but not in index db yet.
         * 2) Timer to actually garbage collect a token file. We do it on timer,
         * so that we can drain concurrent IOs that may still access the old file
         * (gets only), since we are not locking index db on reads
         */
        FdsTimerPtr tc_timer;
        FdsTimerTaskPtr tc_timer_task;
    };

    typedef boost::shared_ptr<TokenCompactor> TokenCompactorPtr;

    class CompactorTimerTask: public FdsTimerTask {
  public:
        TokenCompactor* tok_compactor;

        CompactorTimerTask(FdsTimer &timer, TokenCompactor* tc)  //NOLINT
                : FdsTimerTask(timer) {
            tok_compactor = tc;
        }
        ~CompactorTimerTask() {}

        void runTimerTask();
    };

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_TOKENCOMPACTOR_H_
