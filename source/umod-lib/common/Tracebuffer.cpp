/* Copyright 2014 Formation Data Systems, Inc.*/
#include <string>
#include <fds_module_provider.h>
#include <Tracebuffer.h>

namespace fds {

/**
* @brief Maintains pool of trace buffer entries
*/
TracebufferPool::TracebufferPool(int poolEntryCnt)
{
    pool_ = new TraceEntry[poolEntryCnt];
    freelistHead_ = pool_;

    int i = 0;
    for (i = 0; i < poolEntryCnt-1; i++) {
        pool_[i].next = &(pool_[i+1]);
    }
    pool_[i].next = nullptr;
}

TracebufferPool::~TracebufferPool()
{
    delete pool_;
    pool_ = nullptr;
    freelistHead_ = nullptr;
}

TraceEntry* TracebufferPool::allocTraceEntry()
{
    TraceEntry *item;
    {
        fds_spinlock::scoped_lock g(lock_);
        if (freelistHead_ == nullptr) {
            return nullptr;
        }
        item = freelistHead_;
        freelistHead_ = freelistHead_->next;
    }
    item->reset();
    return item;
}

void TracebufferPool::freeTraceEntry(TraceEntry *e)
{
  fds_spinlock::scoped_lock l(lock_);
    e->next = freelistHead_;
    freelistHead_ = e;
}

Tracebuffer::Tracebuffer(CommonModuleProviderIf* moduleProvider,
                         TracebufferPolicy traceBufPolicy)
 : HasModuleProvider(moduleProvider)
{
    head_ = nullptr;
    tail_ = nullptr;
    traceCntr_ = 0;
}

bool Tracebuffer::trace(const std::string &traceString)
{
    TraceEntry *e = alloc_();
    traceCntr_++;
    if (e == nullptr) {
        return false;
    }

    strncpy(e->buf, traceString.c_str(), TRACE_BUF_LEN);
    e->buf[TRACE_BUF_LEN - 1] = '\0';
    pushBack_(e);
    return true;
}

uint32_t Tracebuffer::size() const
{
    if (head_ == nullptr) {
        return 0;
    }

    auto h = head_;
    auto t = tail_;
    uint32_t sz = 1;
    while (h != t) {
        h = h->next;
        sz++;
    }
    return sz;
}

TraceEntry* Tracebuffer::alloc_()
{
    auto *pool = MODULEPROVIDER()->getTracebufferPool();
    if (traceCntr_ >= MAX_TRACEBUFFER_ENTRY_CNT) {
        return popFront_();
    }

    auto e = pool->allocTraceEntry();
    if (e == nullptr) {
        return popFront_();
    }
    return e;
}

void Tracebuffer::pushBack_(TraceEntry *e)
{
    fds_spinlock::scoped_lock l(lock_);
    if (head_ == nullptr) {
        fds_assert(tail_ == nullptr);
        head_ = tail_ = e;
        e->next = nullptr;
    } else {
        tail_->next = e;
        tail_ = e;
    }
}

TraceEntry* Tracebuffer::popFront_()
{
    fds_spinlock::scoped_lock l(lock_);
    if (head_ == nullptr) {
        fds_assert(tail_ == nullptr);
        return nullptr;
    }

    auto ret = head_;
    head_ = head_->next;
    if (head_ == nullptr) {
        tail_ = nullptr;
    }
    return ret;
}

}  // namespace fds

