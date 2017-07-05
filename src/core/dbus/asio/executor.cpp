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

#include <core/dbus/bus.h>
#include <core/dbus/executor.h>
#include <core/dbus/traits/timeout.h>
#include <core/dbus/traits/watch.h>

#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>

#include <stdexcept>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <future>
#include <thread>

namespace core
{
namespace dbus
{
namespace traits
{
template<>
struct Timeout<DBusTimeout>
{
    typedef int DurationType;

    static inline bool is_timeout_enabled(DBusTimeout* timeout)
    {
        return TRUE == dbus_timeout_get_enabled(timeout);
    }

    static inline int get_timeout_interval(DBusTimeout* timeout)
    {
        return DurationType(dbus_timeout_get_interval(timeout));
    }

    static inline void invoke_timeout_handler(DBusTimeout* timeout)
    {
        dbus_timeout_handle(timeout);
    }
};

template<>
struct Watch<DBusWatch>
{
    inline static int readable_event() { return DBUS_WATCH_READABLE; }
    inline static int writeable_event() { return DBUS_WATCH_WRITABLE; }
    inline static int error_event() { return DBUS_WATCH_ERROR; }
    inline static int hangup_event() { return DBUS_WATCH_HANGUP; }

    static inline bool is_watch_enabled(DBusWatch* watch)
    {
        return TRUE == dbus_watch_get_enabled(watch);
    }

    static inline int get_watch_unix_fd(DBusWatch* watch)
    {
        return dbus_watch_get_unix_fd(watch);
    }

    static inline bool is_watch_monitoring_fd_for_readable(DBusWatch* watch)
    {
        return dbus_watch_get_flags(watch) & DBUS_WATCH_READABLE;
    }

    static bool is_watch_monitoring_fd_for_writable(DBusWatch* watch)
    {
        return dbus_watch_get_flags(watch) & DBUS_WATCH_WRITABLE;
    }

    static bool invoke_watch_handler_for_event(DBusWatch* watch, int event)
    {
        return dbus_watch_handle(watch, event);
    }
};
}
namespace asio
{
class Executor : public core::dbus::Executor
{
public:
    template<typename UnderlyingTimeoutType = DBusTimeout>
    struct Timeout : std::enable_shared_from_this<Timeout<UnderlyingTimeoutType>>
    {
        Timeout(boost::asio::io_service& io_service, UnderlyingTimeoutType* timeout)
            : io_service(io_service),
              timer(io_service),
              timeout(timeout)
        {
            if (!timeout)
                throw std::runtime_error("Precondition violated: timeout has to be non-null");
        }

        ~Timeout()
        {
            // cancel();
        }

        void start()
        {
            if (!traits::Timeout<UnderlyingTimeoutType>::is_timeout_enabled(timeout))
            {
                return;
            }

            // We do not keep ourselves alive to prevent from races during destruction.
            std::weak_ptr<Timeout<UnderlyingTimeoutType>> wp{this->shared_from_this()};

            timer.expires_from_now(
                        boost::posix_time::milliseconds(
                            traits::Timeout<UnderlyingTimeoutType>::get_timeout_interval(
                                timeout)));
            timer.async_wait([wp](const boost::system::error_code& ec)
            {
                auto sp = wp.lock();

                if (sp)
                    sp->on_timeout(ec);
            });
        }

        void cancel()
        {
            try
            {
                timer.cancel();
            } catch(...)
            {
                // Really not sure what we should do about exceptions here.
            }
        }

        void on_timeout(const boost::system::error_code& ec)
        {
            if (ec == boost::asio::error::operation_aborted)
                return;

            if (ec)
                return;

            traits::Timeout<UnderlyingTimeoutType>::invoke_timeout_handler(timeout);
        }

        boost::asio::io_service& io_service;
        boost::asio::deadline_timer timer;
        UnderlyingTimeoutType* timeout;
    };

    template<typename UnderlyingWatchType = DBusWatch>
    struct Watch : std::enable_shared_from_this<Watch<UnderlyingWatchType>>
    {
        Watch(boost::asio::io_service& io_service, UnderlyingWatchType* watch) : io_service(io_service),
            stream_descriptor(io_service),
            watch(watch)
        {
            if (!watch)
                throw std::runtime_error("Precondition violated: watch has to be non-null");
        }

        ~Watch() noexcept
        {
            stream_descriptor.cancel();
            stream_descriptor.release();
        }

        void start()
        {
            stream_descriptor.assign(traits::Watch<UnderlyingWatchType>::get_watch_unix_fd(watch));
            restart();
        }

        void restart()
        {
            // We do not keep ourselves alive to prevent from races during destruction.
            std::weak_ptr<Watch<UnderlyingWatchType>> wp{this->shared_from_this()};

            if (traits::Watch<UnderlyingWatchType>::is_watch_monitoring_fd_for_readable(watch))
            {
                stream_descriptor.async_read_some(boost::asio::null_buffers(), [wp](boost::system::error_code ec, std::size_t bytes_transferred)
                {
                    auto sp = wp.lock();

                    if (sp)
                        sp->on_stream_descriptor_event(
                                    traits::Watch<UnderlyingWatchType>::readable_event(),
                                    ec,
                                    bytes_transferred);
                });
            }

            if (traits::Watch<UnderlyingWatchType>::is_watch_monitoring_fd_for_writable(watch))
            {
                stream_descriptor.async_write_some(boost::asio::null_buffers(), [wp](boost::system::error_code ec, std::size_t bytes_transferred)
                {
                    auto sp = wp.lock();

                    if (sp)
                        sp->on_stream_descriptor_event(
                                    traits::Watch<UnderlyingWatchType>::writeable_event(),
                                    ec,
                                    bytes_transferred);
                });
            }
        }

        void cancel()
        {
            try
            {
                stream_descriptor.cancel();
            }
            catch (...)
            {
            }
        }

        void on_stream_descriptor_event(int event, const boost::system::error_code& error, std::size_t)
        {
            if (error == boost::asio::error::operation_aborted)
            {
                return;
            }

            if (error)
            {
                traits::Watch<UnderlyingWatchType>::invoke_watch_handler_for_event(
                            watch,
                            traits::Watch<UnderlyingWatchType>::error_event());
            }
            else
            {
                if (!traits::Watch<UnderlyingWatchType>::invoke_watch_handler_for_event(watch, event))
                    throw std::runtime_error("Insufficient memory while handling watch event");

                restart();
            }
        }

        boost::asio::io_service& io_service;
        boost::asio::posix::stream_descriptor stream_descriptor;
        UnderlyingWatchType* watch;
    };

    template<typename T>
    struct Holder
    {
        static void ptr_delete(void* p)
        {
            delete static_cast<Holder<T>*>(p);
        }

        Holder(const T& t) : value(t)
        {
        }

        T value;
    };

    static dbus_bool_t on_dbus_add_watch(DBusWatch* watch, void* data)
    {
        if (dbus_watch_get_enabled(watch) == FALSE)
            return TRUE;

        auto thiz = static_cast<Executor*>(data);
        auto w = std::shared_ptr<Watch<>>(new Watch<>(thiz->io_service, watch));
        auto holder = new Holder<std::shared_ptr<Watch<>>>(w);
        dbus_watch_set_data(watch, holder, Holder<std::shared_ptr<Watch<>>>::ptr_delete);

        w->start();

        return TRUE;
    }

    static void on_dbus_remove_watch(DBusWatch* watch, void*)
    {
        auto w = static_cast<Holder<std::shared_ptr<Watch<>>>*>(dbus_watch_get_data(watch));
        if (!w)
            return;
        w->value->cancel();
    }

    static void on_dbus_watch_toggled(DBusWatch* watch, void*)
    {
        auto holder = static_cast<Holder<std::shared_ptr<Watch<>>>*>(dbus_watch_get_data(watch));
        if (!holder)
            return;
        dbus_watch_get_enabled(watch) == TRUE ? holder->value->restart() : holder->value->cancel();
    }

    static dbus_bool_t on_dbus_add_timeout(DBusTimeout* timeout, void* data)
    {
        auto thiz = static_cast<Executor*>(data);
        auto t = std::shared_ptr<Timeout<>>(new Timeout<>(thiz->io_service, timeout));
        auto holder = new Holder<std::shared_ptr<Timeout<>>>(t);
        dbus_timeout_set_data(
                    timeout,
                    holder,
                    Holder<std::shared_ptr<Timeout<>>>::ptr_delete);

        t->start();
        return TRUE;
    }

    static void on_dbus_remove_timeout(DBusTimeout* timeout, void*)
    {
        static_cast<Holder<std::shared_ptr<Timeout<>>>*>(dbus_timeout_get_data(timeout))->value->cancel();
    }

    static void on_dbus_timeout_toggled(DBusTimeout* timeout, void*)
    {
        auto holder = static_cast<Holder<std::shared_ptr<Timeout<>>>*>(dbus_timeout_get_data(timeout));
        holder->value->start();
    }

    static void on_dbus_wakeup_event_loop(void* data)
    {
        auto thiz = static_cast<Executor*>(data);
        auto bus = thiz->bus;
        thiz->io_service.post([bus]()
        {
            while (dbus_connection_get_dispatch_status(bus->raw()) == DBUS_DISPATCH_DATA_REMAINS)
            {
                dbus_connection_dispatch(bus->raw());
            }
        });
    }

public:

    Executor(const Bus::Ptr& bus, boost::asio::io_service& io) : bus(bus), io_service(io), work(io_service)
    {
        if (!bus)
            throw std::runtime_error("Precondition violated, cannot construct executor for null bus.");

        if (!dbus_connection_set_watch_functions(
                    bus->raw(),
                    on_dbus_add_watch,
                    on_dbus_remove_watch,
                    on_dbus_watch_toggled,
                    this,
                    nullptr))
            throw std::runtime_error("Problem installing watch functions.");

        if (!dbus_connection_set_timeout_functions(
                    bus->raw(),
                    on_dbus_add_timeout,
                    on_dbus_remove_timeout,
                    on_dbus_timeout_toggled,
                    this,
                    nullptr))
            throw std::runtime_error("Problem installing timeout functions.");

        dbus_connection_set_wakeup_main_function(
                    bus->raw(),
                    on_dbus_wakeup_event_loop,
                    this,
                    nullptr);
    }

    ~Executor() noexcept
    {
        stop();
    }

    void run()
    {
        io_service.run();
    }

    void stop()
    {
        io_service.stop();
    }

private:
    Bus::Ptr bus;
    boost::asio::io_service& io_service;
    boost::asio::io_service::work work;
};

ORG_FREEDESKTOP_DBUS_DLL_PUBLIC Executor::Ptr make_executor(const Bus::Ptr& bus)
{
    static boost::asio::io_service io;
    return std::make_shared<core::dbus::asio::Executor>(bus, io);
}

ORG_FREEDESKTOP_DBUS_DLL_PUBLIC Executor::Ptr make_executor(const Bus::Ptr& bus, boost::asio::io_service& io)
{
    return std::make_shared<core::dbus::asio::Executor>(bus, io);
}

}
}
}

