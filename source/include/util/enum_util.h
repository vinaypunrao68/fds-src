#ifndef ENUM_UTIL_H
#define ENUM_UTIL_H

#include <boost/preprocessor.hpp>
#include <algorithm>

#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE(r, data, elem)    \
    case data::elem :                                                         \
                converted += "";                                              \
                converted += BOOST_PP_STRINGIZE(elem);                        \
                std::transform(converted.begin(), converted.end(),            \
                converted.begin(), ::tolower);                                \
                return converted;

#define DEFINE_ENUM_WITH_STRING_CONVERSIONS(name, enumerators)                \
    enum class name {                                                         \
        BOOST_PP_SEQ_ENUM(enumerators(__MAX__))                               \
    };                                                                        \
                                                                              \
    inline std::string EnumToString(name v)                                   \
    {                                                                         \
        std::string converted {""};                                           \
        switch (v)                                                            \
        {                                                                     \
            BOOST_PP_SEQ_FOR_EACH(                                            \
                X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,          \
                name,                                                         \
                enumerators                                                   \
            )                                                                 \
            default: return "[Unknown " BOOST_PP_STRINGIZE(name) "]";         \
        }                                                                     \
    }                                                                         \
                                                                              \
    inline std::ostream& operator<<(std::ostream& out, name v)                \
    {                                                                         \
        out << EnumToString(v);                                               \
        return out;                                                           \
    }                                                                         \
                                                                              
namespace fds_enum {

    template<typename T>
    constexpr 
    size_t get_size() {
        return static_cast<size_t>(T::__MAX__);
    }                   

    template<typename T>
    constexpr 
    size_t get_index(const T& x) {
        return static_cast<size_t>(x);
    }                   

};


#endif // ENUM_UTIL_H

