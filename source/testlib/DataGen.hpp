/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_DATAGEN_H_  // NOLINT
#define SOURCE_INCLUDE_DATAGEN_H_

#include <string>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace fds {

typedef boost::shared_ptr<std::string> StringPtr;

/**
* @brief Generic data generation interface
*/
struct DataGenIf {
    virtual ~DataGenIf() {}
    virtual StringPtr nextItem() = 0;
    virtual bool hasNext()
    {
        return true;
    }
};
typedef boost::shared_ptr<DataGenIf> DataGenIfPtr;

/**
* @brief Functor for generating alpha numeric string
*/
struct RandAlphaNumStringF
{
    StringPtr operator() (const size_t &sz)
    {
        auto randchar = []() -> char
        {
            const char charset[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
            const size_t max_index = (sizeof(charset) - 1);
            return charset[ rand() % max_index ];  // NOLINT
        };
        auto str = boost::make_shared<std::string>(sz, 0);
        std::generate_n(str->begin(), sz, randchar);
        return str;
    }
};

/**
* @brief Generates random data
*
* @tparam RandDataFuncT Function that generates random data.  This
* functor must take size as argument
*/
template <class RandDataFuncT = RandAlphaNumStringF>
struct RandDataGenerator : DataGenIf
{
    RandDataGenerator(size_t min, size_t max)
    {
        min_ = min;
        max_ = max;
    }
    virtual StringPtr nextItem() override
    {
        size_t sz = (rand() % ((max_ - min_)+1)) + min_;  // NOLINT
        return randDataF_(sz);
    }
 protected:
    size_t min_;
    size_t max_;
    RandDataFuncT randDataF_;
};

/**
* @brief Generates random data and caches it.  Use it if you don't want to incur generation
* cost
*
* @tparam RandDataFuncT Function that generates random data.  This
* functor must take size as argument
*/
template <class RandDataFuncT = RandAlphaNumStringF>
struct CachedRandDataGenerator : DataGenIf
{
    /**
    * @brief Constructor
    *
    * @param count Number of items in the cache
    * @param wrapAround To wrap around once the last element is reached when nextItem()
    * is called
    * @param min minumum size of data to generate
    * @param max maximum size of data to generate
    */
    CachedRandDataGenerator(int count, bool wrapAround, size_t min, size_t max)
    {
        items_.resize(count);
        for (int i = 0; i < count; i++) {
            size_t sz = (rand() % ((max - min)+1)) + min;  // NOLINT
            items_[i] = randDataF_(sz);
        }
    }
    virtual StringPtr nextItem() override
    {
        auto idx = idx_++;
        if (wrapAround_) {
            idx %= items_.size();
        }
        return items_[idx];
    }
    virtual bool hasNext()
    {
        if (wrapAround_) {
            return true;
        }
        return idx_ < items_.size();
    }
 protected:
    /* Cached items */
    std::vector<StringPtr> items_;
    /* Current index. nextItem() will return item from this index */
    uint32_t idx_;
    /* To wrap around the items_ array or not */
    bool wrapAround_;
    /* Functor to generate data */
    RandDataFuncT randDataF_;
};

/**
* @brief For generating svc messages
*
* @tparam SvcMsgGenFuncT Functor for generating service message
*/
template <typename SvcMsgGenFuncT>
struct SvcMsgGenerator : DataGenIf
{
    SvcMsgGenerator()
    {
    }
    virtual StringPtr nextItem() override
    {
        return nullptr;
    }

 protected:
    SvcMsgGenFuncT svcMsgF_;
};
}  // namespace fds

#endif  // SOURCE_INCLUDE_DATAGEN_H_  // NOLINT
