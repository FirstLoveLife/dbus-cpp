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
#ifndef CORE_DBUS_TRAITS_WATCH_H_
#define CORE_DBUS_TRAITS_WATCH_H_

#include <chrono>

namespace core
{
namespace dbus
{
namespace traits
{
template<typename UnderlyingType>
struct Watch
{
    static inline int readable_event() { return -1; }
    static inline int writeable_event() { return -1; }
    static inline int error_event() { return -1; }
    static inline int hangup_event() { return -1; }

    static bool is_watch_enabled(UnderlyingType* watch);
    static int get_watch_unix_fd(UnderlyingType* watch);
    static bool is_watch_monitoring_fd_for_readable(UnderlyingType* watch);
    static bool is_watch_monitoring_fd_for_writable(UnderlyingType* watch);
    static bool invoke_watch_handler_for_event(UnderlyingType* watch, int event);
};
}
}
}

#endif // CORE_DBUS_TRAITS_WATCH_H_
