#ifndef _BEHAVIOR_H_
#define _BEHAVIOR_H_

#include <functional>
#include <unordered_map>

namespace fds {

template<class KeyT, class ArgT>
struct Behavior {
    using Handler = std::function<void(const SHPTR<ArgT>&)>;
    using BehaviorItem = std::pair<KeyT, Handler>;
    Behavior(const std::string &name) {
        this->name_ = name;
    }
    Behavior(const std::string &name, const std::vector<BehaviorItem>& items) {
        this->name_ = name;
        this->operator=(items);
    }
    void operator = (const std::vector<BehaviorItem>& items)
    {
        handlers_.clear();
        this->operator+=(items);
    }
    void operator += (const std::vector<BehaviorItem>& items)
    {
        for (auto &item : items) {
            handlers_[item.first] = item.second;
        }
    }
    void handle(const KeyT& key)
    {
        auto itr = handlers_.find(key);
        if (itr != handlers_.end()) {
            (itr->second)();
        } else {
            // TODO: We shouldn't panic here
            fds_panic("Unhandled message");
        }
    }
    const std::string& logString() const {
        return name_;
    }

 protected:
    std::unordered_map<KeyT, Handler> handlers_;
    std::string name_;
};

template <class KeyT, class ArgT>
struct OnMsg {
    OnMsg(const KeyT &k) {
        this->key = k;
    }
    typename Behavior<KeyT, ArgT>::BehaviorItem operator>>(
        typename Behavior<KeyT, ArgT>::Handler handler) {
        return {key, handler};
    }
    KeyT            key;
};

}  // namespace fds

#endif
