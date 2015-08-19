#ifndef ENUM_UTIL_H
#define ENUM_UTIL_H

#include <boost/preprocessor.hpp>
#include <algorithm>

#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE(r, data, elem)    \
    case data::elem :                                                         \
                {                                                             \
                    static const std::string stringized =                     \
                                BOOST_PP_STRINGIZE(elem);                     \
                    return stringized;                                        \
                }

#define DEFINE_ENUM_WITH_STRING_CONVERSIONS(name, enumerators)                \
    enum class name {                                                         \
        BOOST_PP_SEQ_ENUM(enumerators(__MAX__))                               \
    };                                                                        \
                                                                              \
    inline std::string EnumToString(name v)                         \
    {                                                                         \
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


/**
 * The following utilities enable the definition of enums in which a starting
 * value can be set and textual descriptions (aside from the enum value name itself)
 * can be defined.
 */

/**
 * First member of the pair is the enum name. The second member is an optional
 * textual description.
 */
using enumDescription = std::pair<std::string, std::string>;

extern std::map<int, enumDescription>& operator, (std::map<int, enumDescription>& dest,
                                                  const std::pair<int, enumDescription>& entry);

/**
 * Use this macro to add items to the enum description map.
 * name: Enum member name.
 * value: An optional starting value for the enum item of the form "= 500".
 * desc: An optional string description of the enum item.
 */
#define ADD_TO_MAP(name, value, desc) std::pair<int, enumDescription>(name, enumDescription(#name, desc))

/**
 * Use this macro to add items to the enum definition.
 */
#define ADD_TO_ENUM(name, value, desc) name value

/**
 * Use this macro to populate the enum description map outside any method.
 */
#define MAKE_ENUM_MAP_GLOBAL(mapName, values) \
    int __makeMap##mapName() {mapName, values(ADD_TO_MAP); return 0;}  \
    int __makeMapTmp##mapName = __makeMap##mapName();

/**
 * Use this macro to populate the enum description within a method.
 */
#define MAKE_ENUM_MAP(mapName, values) mapName, values(ADD_TO_MAP);

/**
 * For example:
 */
// #include "EnumUtils.h*
//
// // List the desired enumeration items, their values and descriptions.
// #define MyEnumValues(ADD) ADD(val1, , "First value."), ADD(val2, = 50," "), ADD(val3, = 100, "Third value."), ADD(val4, ," ")
//
// // Define the enum.
// enum MyEnum {
//     MyEnumValues(ADD_TO_ENUM)
// };
//
// // Define the enum description map.
// std::map<int, enumDescription> MyEnumStrings;
//
// // This is how you initialize the enum description map outside any function.
// MAKE_ENUM_MAP_GLOBAL(MyEnumStrings, MyEnumValues);
//
// void MyInitializationMethod()
// {
//     // Or you can initialize it inside one of your functions/methods
//     MAKE_ENUM_MAP(MyEnumStrings, MyEnumValues);
// }

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

