/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */
#ifndef CORE_DBUS_HELPER_SIGNATURE_H_
#define CORE_DBUS_HELPER_SIGNATURE_H_

#include <core/dbus/helper/type_mapper.h>

#include <string>

namespace core
{
namespace dbus
{
namespace helper
{
template<typename T>
inline std::string atomic_signature()
{
    return TypeMapper<T>::signature();
}

inline std::string signature()
{
    static const std::string s;
    return s;
}

template<typename Arg, typename... Args>
inline std::string signature(const Arg&, const Args& ... remainder)
{
    static const std::string s = atomic_signature<Arg>() + signature(remainder...);
    return s;
}
}
}
}

#endif // CORE_DBUS_HELPER_SIGNATURE_H_
