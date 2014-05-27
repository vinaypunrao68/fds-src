/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <atomic>
#include <thread>
#include <sstream>
#include <string>
#include <functional>
#include <qos_ctrl.h>
#include <concurrency/ThreadPool.h>
#include <TransJournal.h>
#include <hash/MurmurHash3.h>
#include <fds_process.h>

namespace fds {

typedef boost::shared_ptr<TransJournal<ObjectID, ObjectIdJrnlEntry> > TransJournalPtr;

class MockIo : public FDS_IOType
{
public:
    MockIo() {
        trans_id_ = 0;
    }
    virtual ~MockIo() {}
    void setTransId(const TransJournalId& trans_id)
    {
        trans_id_ = trans_id;
    }

    TransJournalId trans_id_;
};

class ObjectIOGenerator {
public:
    ObjectIOGenerator(TransJournalPtr jrnl,
            fds_threadpoolPtr threadpool,
            const int &io_cnt)
    : jrnl_(jrnl),
      threadpool_(threadpool),
      io_cnt_(io_cnt)
    {

    }

    void create_transaction_cb(MockIo* ioReq, TransJournalId trans_id)
    {
        ioReq->setTransId(trans_id);
    }

    void start()
    {
        std::stringstream ss;
        ss << "hello" << rand() % 100;
        std::string content = ss.str();

        ObjectID obj_id;
        int qd_cnt = 0;
        MurmurHash3_x64_128(content.c_str(),
                       content.size(),
                       0,
                       &obj_id);
        for (int i = 0; i < io_cnt_; i++) {
            fds_assert(obj_id != NullObjectID);
            TransJournalId trans_id;
            MockIo *ioReq = new MockIo();
            Error err = jrnl_->create_transaction(obj_id,
                    static_cast<FDS_IOType *>(ioReq), trans_id,
                    std::bind(&ObjectIOGenerator::create_transaction_cb, this,
                            ioReq, std::placeholders::_1));
            if (err != ERR_OK &&
                    err != ERR_TRANS_JOURNAL_REQUEST_QUEUED) {
                fds_verify (!"error to create_transaction");

            }

            if (err == ERR_TRANS_JOURNAL_REQUEST_QUEUED) {
                qd_cnt++;
                continue;
            }
            threadpool_->schedule(&ObjectIOGenerator::handle_io, this,
                    static_cast<FDS_IOType *>(ioReq));
        }
        std::cout << "qd cnt " << qd_cnt << std::endl;
    }

    void handle_io(FDS_IOType *io)
    {
        MockIo *ioReq = static_cast<MockIo *>(io);
        // usleep(1);
        LOGDEBUG << "Invoking release of id: " << ioReq->trans_id_;
        jrnl_->release_transaction(ioReq->trans_id_);
        completed_io_cnt_++;
        delete ioReq;
    }

    int get_completed_io_cnt() {
        return completed_io_cnt_;
    }
private:
    TransJournalPtr jrnl_;
    fds_threadpoolPtr threadpool_;
    int io_cnt_;
    std::atomic<int> completed_io_cnt_;
};
typedef boost::shared_ptr<ObjectIOGenerator> ObjectIOGeneratorPtr;

/**
 * Mocks priocesIO in FDS_QosControl
 */
class MockQosCtrl : public FDS_QoSControl {
public:
    MockQosCtrl(fds_threadpoolPtr threadpool)
    : threadpool_(threadpool)
    {
    }
    void setIoGenerator(ObjectIOGeneratorPtr io_handler)
    {
        io_handler_ = io_handler;
    }
    virtual ~MockQosCtrl(){}
    virtual Error enqueueIO(fds_volid_t volUUID, FDS_IOType *io) override 
    {
        threadpool_->schedule(&ObjectIOGenerator::handle_io, io_handler_.get(),io);
        return ERR_OK;
    }

private:
    fds_threadpoolPtr threadpool_;
    ObjectIOGeneratorPtr io_handler_;
};
typedef boost::shared_ptr<MockQosCtrl> MockQosCtrlPtr;

class TransJournalUt {
public:
    bool test_concurrent_puts()
    {
        int io_cnt = 1000;
        int thread_cnt = 100;
        fds_threadpoolPtr threadpool;
        TransJournalPtr jrnl;
        threadpool.reset(new fds_threadpool());
        g_fdslog->setSeverityFilter((fds_log::severity_level) 0);
        MockQosCtrlPtr qos_ctrl(new MockQosCtrl(threadpool));
        jrnl.reset(new TransJournal<ObjectID, ObjectIdJrnlEntry>(io_cnt * thread_cnt, qos_ctrl.get(), g_fdslog));
        ObjectIOGeneratorPtr io_generator(new ObjectIOGenerator(jrnl, threadpool, io_cnt));
        qos_ctrl->setIoGenerator(io_generator);

        std::vector<std::thread*> threads;
        for (int i = 0; i < thread_cnt; i++) {
            std::thread* t = new std::thread(&ObjectIOGenerator::start,
                    io_generator.get());
            threads.push_back(t);
        }

        /* Wait for migration to complete */
        int slept_time = 0;
        while (io_generator->get_completed_io_cnt() < (io_cnt * thread_cnt) &&
                slept_time < 20) {
            sleep(1);
            slept_time++;
        }

        /* Check the queue */
        for (auto itr : jrnl->_free_trans_ids) {
            fds_verify(itr < jrnl->_max_journal_entries);
        }

        fds_verify(io_generator->get_completed_io_cnt() == (io_cnt * thread_cnt));
        fds_verify(jrnl->get_active_cnt() == 0);
        fds_verify(jrnl->get_pending_cnt() == 0);

        std::cout << "Reschedule cnt: " << jrnl->_rescheduled_cnt << std::endl;
        std::cout << "Test " << __FUNCTION__ << " passed\n";

        for (int i = 0; i < thread_cnt; i++) {
            threads[i]->join();
            delete threads[i];
        }
        return true;
    }

private:
};
}  // namespace fds

int main() {
    init_process_globals("TransJournalUt.log");
    srand (time(NULL));
    TransJournalUt ut;
    ut.test_concurrent_puts();
}
