/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_UTIL_EVENTTRACKER_H_
#define SOURCE_INCLUDE_UTIL_EVENTTRACKER_H_

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

namespace fds
{

template <typename Service>
struct TrackerBase {
    TrackerBase() = default;
    TrackerBase(TrackerBase const&) = default;
    TrackerBase& operator=(TrackerBase const&) = default;
    virtual ~TrackerBase() = default;

    virtual void poke() = 0;
    virtual TrackerBase* clone() = 0;

    Service service;
};

template <typename Callback, typename Service, typename Duration = std::chrono::seconds>
struct TrackerMap : public TrackerBase<Service> {

    template<typename T>
    using unique            = std::unique_ptr<T>;
    using callback_type     = Callback;
    using duration_type     = Duration;
    using service_type      = Service;
    using type              = TrackerMap<callback_type, service_type, duration_type>;
    using count_type        = size_t;
    using clock_type        = std::chrono::system_clock;
    using time_point_type   = std::chrono::time_point<clock_type, duration_type>;
    using bucket_type       = std::map<time_point_type, count_type>;

    TrackerMap(callback_type& c, size_t _ticks, size_t _threshold) :
        callback(c), counters(), tick_window(_ticks), threshold(_threshold)
    {}

    TrackerMap(callback_type&& c, size_t _ticks, size_t _threshold) :
        callback(std::move(c)), counters(), tick_window(_ticks), threshold(_threshold)
    {}

    TrackerMap(TrackerMap const& rhs) :
        callback(rhs.callback), counters(), tick_window(rhs.tick_window), threshold(rhs.threshold) {
    }

    TrackerMap& operator=(TrackerMap const&) = delete;
    ~TrackerMap() = default;

    void poke() override
    {
        auto const window = duration_type(tick_window);
        auto now = std::chrono::time_point_cast<duration_type>(clock_type::now());

        // We don't care about anything that happened prior to this timepoint.
        auto window_start = now - window;

        // Remove any counters that are older than the number of ticks in our
        // window.
        auto it = counters.lower_bound(window_start);
        counters.erase(counters.begin(), it);

        // Get the bucket for this timepoint.
        it = counters.find(now);
        if (counters.end() == it) {
            auto r = counters.insert(std::make_pair(std::move(now), 0u));
            it = r.first; // If insert fails, element existed
        }
        // Increment the count.
        ++it->second;

        // Tally up the number of events and call the callback if the threshold
        // is reached.
        size_t events = 0;
        for (auto& c : counters) {
            events += c.second;
        }

        if (events >= threshold) {
            callback(this->service, events);
        }
    }

    virtual type* clone() override
    { return new type(*this); }

 private:
    callback_type   callback;
    bucket_type     counters;
    size_t const    tick_window;
    size_t const    threshold;
};


template <typename K, typename E, typename HK = std::hash<K>, typename HE = std::hash<E>>
class EventTracker {
    using event_type  = E;
    using event_hash_type = HE;
    using service_key_type  = K;
    using service_hash_type = HK;
    template<typename T>
    using unique            = std::unique_ptr<T>;
    using tracker_type      = TrackerBase<service_key_type>;
    using event_map_type    = std::unordered_map<event_type, unique<tracker_type>, event_hash_type>;
    using service_map_type  = std::unordered_map<service_key_type, unique<event_map_type>, service_hash_type>;

 public:
    EventTracker() = default;
    ~EventTracker() = default;

    EventTracker(EventTracker const& rhs) = delete;
    EventTracker& operator=(EventTracker const& rhs) = delete;

    bool register_event(event_type const event, unique<tracker_type>&& tracker) {
        return registered_events.insert(std::make_pair(std::move(event), std::move(tracker))).second;
    }

    // Right now this whole method is protected by a single mutex. This was the
    // easiest option, and unless it causes too much contention is the easiest
    // to verify.
    void feed_event(event_type const event, service_key_type service) {
        {
            auto guard = WriteGuard(service_map_lock);
            auto it = service_map.find(service);
            if (service_map.end() == it) {
                // Add a new service
                unique<event_map_type> new_map(new event_map_type);
                for (auto& p : registered_events) {
                    unique<tracker_type> tracker(p.second->clone());
                    tracker->service = service;
                    new_map->insert(std::make_pair(p.first, std::move(tracker)));
                }
                auto r = service_map.insert(std::make_pair(service, std::move(new_map)));
                it = r.first; // If insert fails, element existed
            }

            auto track_it = it->second->find(event);
            if (it->second->end() == track_it) {
                // Need to add a tracker for this service, copy it from
                // the registered list
                try {
                    unique<tracker_type> tracker(registered_events.at(event)->clone());
                    tracker->service = service;
                    auto r = it->second->insert(std::make_pair(event, std::move(tracker)));
                    track_it = r.first; // If insert fails, element existed
                } catch (std::out_of_range& e) {
                    LOGERROR << " tried to track an unregistered event: " << event;
                    return;
                }
            }
            // We've got it, POKE
            track_it->second->poke();
        }
    }

 private:
    fds_rwlock          service_map_lock;
    service_map_type    service_map;
    event_map_type      registered_events;
};
}  // namespace fds
#endif  // SOURCE_INCLUDE_UTIL_EVENTTRACKER_H_
