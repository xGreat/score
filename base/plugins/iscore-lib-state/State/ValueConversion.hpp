#pragma once
#include <QChar>
#include <QDebug>
#include <QJsonValue>
#include <QString>
#include <QVariant>
#include <State/Value.hpp>

class QStringList;

namespace State
{
namespace convert
{

template <typename To>
To value(const ossia::value& val)
{
  static_assert(sizeof(To) == -1, "Type not supported.");
}

template <>
ISCORE_LIB_STATE_EXPORT QVariant value(const ossia::value& val);
template <>
ISCORE_LIB_STATE_EXPORT QJsonValue value(const ossia::value& val);
template <>
ISCORE_LIB_STATE_EXPORT int value(const ossia::value& val);
template <>
ISCORE_LIB_STATE_EXPORT float value(const ossia::value& val);
template <>
ISCORE_LIB_STATE_EXPORT bool value(const ossia::value& val);
template <>
ISCORE_LIB_STATE_EXPORT double value(const ossia::value& val);
template <>
ISCORE_LIB_STATE_EXPORT QString value(const ossia::value& val);
template <>
ISCORE_LIB_STATE_EXPORT QChar value(const ossia::value& val);
template <>
ISCORE_LIB_STATE_EXPORT std::string value(const ossia::value& val);
template <>
ISCORE_LIB_STATE_EXPORT char value(const ossia::value& val);
template <>
ISCORE_LIB_STATE_EXPORT vec2f value(const ossia::value& val);
template <>
ISCORE_LIB_STATE_EXPORT vec3f value(const ossia::value& val);
template <>
ISCORE_LIB_STATE_EXPORT vec4f value(const ossia::value& val);
template <>
ISCORE_LIB_STATE_EXPORT tuple_t value(const ossia::value& val);

ISCORE_LIB_STATE_EXPORT bool
convert(const ossia::value& orig, ossia::value& toConvert);

// Adornishments to allow to differentiate between different value types, e.g.
// 'a', ['a', 12], or "str" for a string.
ISCORE_LIB_STATE_EXPORT QString toPrettyString(const ossia::value& val);

// We require the type to crrectly read back (e.g. int / float / char)
// and as an optimization, since we may need it multiple times,
// we chose to leave the caller save it however he wants. Hence the specific
// API.
ISCORE_LIB_STATE_EXPORT QString
textualType(const ossia::value& val); // For JSONValue serialization
ISCORE_LIB_STATE_EXPORT ossia::value fromQVariant(const QVariant& val);
ISCORE_LIB_STATE_EXPORT ossia::value
fromQJsonValue(const QJsonValue& val); // Best effort
ISCORE_LIB_STATE_EXPORT ossia::value
fromQJsonValue(const QJsonValue& val, ossia::val_type type);
ISCORE_LIB_STATE_EXPORT ossia::value
fromQJsonValue(const QJsonValue& val, const QString& type);

ISCORE_LIB_STATE_EXPORT QString
prettyType(const ossia::value& val); // For display to the user, translated
ISCORE_LIB_STATE_EXPORT QString
    prettyType(ossia::val_type); // For display to the user, translated
ISCORE_LIB_STATE_EXPORT const std::array<const QString, 11>&
ValuePrettyTypesArray(); // For display to the user, translated
ISCORE_LIB_STATE_EXPORT const QStringList&
ValuePrettyTypesList(); // For display to the user, translated
ISCORE_LIB_STATE_EXPORT const
    std::array<std::pair<QString, ossia::val_type>, 10>&
    ValuePrettyTypesMap();
}
}
