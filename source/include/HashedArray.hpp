#ifndef INCLUDE_HASHED_ARRAY_H
#define INCLUDE_HASHED_ARRAY_H

#include <vector>
#include <functional>
#include <boost/noncopyable.hpp>

namespace fds {

/**
* @brief
* @tparam TypeToStore - type to store
* @tparam TypeToHash  - type to hash on
* @tparam HashFunctorT functor that defines operator(TypeToHash&)
*/
template <class TypeToStore, class TypeToHash, class HashFunctorT = std::hash<TypeToHash>>
class HashedArray : boost::noncopyable
{
 public:
    explicit HashedArray(const uint32_t &tblSize)
        : array_(tblSize)
    {
    }

    TypeToStore& operator[](const TypeToHash& obj)
    {
        size_t idx = (hashFunctor_(obj) % array_.size());
        return array_[idx];
    }
 protected:
    HashFunctorT hashFunctor_;
    /* Lock around taskMap_ */    
    std::vector<TypeToStore> array_;
};

}  // namespace fds
#endif   // INCLUDE_HASHED_ARRAY_H
