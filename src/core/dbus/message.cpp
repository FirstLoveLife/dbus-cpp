/*
 * Copyright © 2012 Canonical Ltd->
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

#include <core/dbus/message.h>

#include <core/dbus/codec.h>
#include <core/dbus/error.h>
#include <core/dbus/visibility.h>

#include <core/dbus/types/object_path.h>

#include "message_p.h"

#include <dbus/dbus.h>

#include <exception>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <stack>
#include <stdexcept>

namespace core
{
namespace dbus
{
Message::Reader::Reader()
{
}

Message::Reader::Reader(const std::shared_ptr<Message>& msg)
    : d(new Private(msg))
{
    if (!msg)
        throw std::runtime_error(
                "Precondition violated, cannot construct Reader for null message.");
}

Message::Reader::Reader(Message::Reader&& that) : d(std::move(that.d))
{
}

Message::Reader& Message::Reader::operator=(Message::Reader&& rhs)
{
    d = std::move(rhs.d);
    return *this;
}

Message::Reader::~Reader()
{
}

ArgumentType Message::Reader::type() const
{
    return static_cast<ArgumentType>(
                dbus_message_iter_get_arg_type(
                    std::addressof(d->iter)));
}

void Message::Reader::pop()
{
    dbus_message_iter_next(std::addressof(d->iter));
}

std::int8_t Message::Reader::pop_byte()
{
    d->ensure_argument_type_or_throw(ArgumentType::byte);

    std::int8_t result;
    dbus_message_iter_get_basic(
                std::addressof(d->iter),
                std::addressof(result));
    dbus_message_iter_next(std::addressof(d->iter));
    return result;
}

bool Message::Reader::pop_boolean()
{
    d->ensure_argument_type_or_throw(ArgumentType::boolean);

    dbus_bool_t result;
    dbus_message_iter_get_basic(
                std::addressof(d->iter),
                std::addressof(result));
    dbus_message_iter_next(std::addressof(d->iter));
    return !!result;
}

std::int16_t Message::Reader::pop_int16()
{
    d->ensure_argument_type_or_throw(ArgumentType::int16);

    std::int16_t result;
    dbus_message_iter_get_basic(
                std::addressof(d->iter),
                std::addressof(result));
    dbus_message_iter_next(std::addressof(d->iter));
    return result;
}

std::uint16_t Message::Reader::pop_uint16()
{
    d->ensure_argument_type_or_throw(ArgumentType::uint16);

    std::uint16_t result;
    dbus_message_iter_get_basic(
                std::addressof(d->iter),
                std::addressof(result));
    dbus_message_iter_next(std::addressof(d->iter));
    return result;
}

std::int32_t Message::Reader::pop_int32()
{
    d->ensure_argument_type_or_throw(ArgumentType::int32);

    std::int32_t result;
    dbus_message_iter_get_basic(
                std::addressof(d->iter),
                std::addressof(result));
    dbus_message_iter_next(std::addressof(d->iter));
    return result;
}

std::uint32_t Message::Reader::pop_uint32()
{
    d->ensure_argument_type_or_throw(ArgumentType::uint32);
    std::uint32_t result;
    dbus_message_iter_get_basic(
                std::addressof(d->iter),
                std::addressof(result));
    dbus_message_iter_next(std::addressof(d->iter));
    return result;
}

std::int64_t Message::Reader::pop_int64()
{
    d->ensure_argument_type_or_throw(ArgumentType::int64);

    std::int64_t result;
    dbus_message_iter_get_basic(
                std::addressof(d->iter),
                std::addressof(result));
    dbus_message_iter_next(std::addressof(d->iter));
    return result;
}

std::uint64_t Message::Reader::pop_uint64()
{
    d->ensure_argument_type_or_throw(ArgumentType::uint64);

    std::uint64_t result;
    dbus_message_iter_get_basic(
                std::addressof(d->iter),
                std::addressof(result));
    dbus_message_iter_next(std::addressof(d->iter));
    return result;
}

double Message::Reader::pop_floating_point()
{
    d->ensure_argument_type_or_throw(ArgumentType::floating_point);

    double result;
    dbus_message_iter_get_basic(
                std::addressof(d->iter),
                std::addressof(result));
    dbus_message_iter_next(std::addressof(d->iter));
    return result;
}

const char* Message::Reader::pop_string()
{
    d->ensure_argument_type_or_throw(ArgumentType::string);

    return d->pop_string_unchecked();
}

types::ObjectPath Message::Reader::pop_object_path()
{
    d->ensure_argument_type_or_throw(ArgumentType::object_path);

    return types::ObjectPath(d->pop_string_unchecked());
}

types::Signature Message::Reader::pop_signature()
{
    d->ensure_argument_type_or_throw(ArgumentType::signature);

    return types::Signature(d->pop_string_unchecked());
}

types::UnixFd Message::Reader::pop_unix_fd()
{
    d->ensure_argument_type_or_throw(ArgumentType::unix_fd);

    int result = -1;
    dbus_message_iter_get_basic(
                std::addressof(d->iter),
                std::addressof(result));
    dbus_message_iter_next(std::addressof(d->iter));
    return types::UnixFd(result);
}

Message::Reader Message::Reader::pop_array()
{
    Reader result(d->msg);
    dbus_message_iter_recurse(
                std::addressof(d->iter),
                std::addressof(result.d->iter));
    dbus_message_iter_next(std::addressof(d->iter));
    return result;
}

Message::Reader Message::Reader::pop_structure()
{
    Reader result(d->msg);
    dbus_message_iter_recurse(
                std::addressof(d->iter),
                std::addressof(result.d->iter));
    dbus_message_iter_next(std::addressof(d->iter));
    return result;
}

Message::Reader Message::Reader::pop_variant()
{
    Reader result(d->msg);
    dbus_message_iter_recurse(
                std::addressof(d->iter),
                std::addressof(result.d->iter));
    dbus_message_iter_next(std::addressof(d->iter));
    return result;
}

Message::Reader Message::Reader::pop_dict_entry()
{
    Reader result(d->msg);
    dbus_message_iter_recurse(
                std::addressof(d->iter),
                std::addressof(result.d->iter));
    dbus_message_iter_next(std::addressof(d->iter));
    return result;
}

const std::shared_ptr<Message>& Message::Reader::access_message()
{
    return d->msg;
}

Message::Writer::Writer(const std::shared_ptr<Message>& msg)
    : d(new Private{msg, DBusMessageIter()})
{
    if (!msg)
        throw std::runtime_error(
                "Precondition violated, cannot construct Writer for null message.");

    ::memset(std::addressof(d->iter), 0, sizeof(d->iter));
}

Message::Writer::~Writer()
{
}

Message::Writer::Writer(Message::Writer&& that) : d(std::move(that.d))
{
}

Message::Writer& Message::Writer::operator=(Message::Writer&& rhs)
{
    d = std::move(rhs.d);
    return *this;
}

void Message::Writer::push_byte(std::int8_t value)
{
    if (!dbus_message_iter_append_basic(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::byte),
                std::addressof(value)))
        throw std::runtime_error("Not enough memory to append data to message.");
}

void Message::Writer::push_boolean(bool value)
{
    auto bool_value = value ? TRUE : FALSE;

    if (!dbus_message_iter_append_basic(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::boolean),
                std::addressof(bool_value)))
        throw std::runtime_error("Not enough memory to append data to message.");
}

void Message::Writer::push_int16(std::int16_t value)
{
    if (!dbus_message_iter_append_basic(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::int16),
                std::addressof(value)))
        throw std::runtime_error("Not enough memory to append data to message.");
}

void Message::Writer::push_uint16(std::uint16_t value)
{
    if (!dbus_message_iter_append_basic(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::uint16),
                std::addressof(value)))
        throw std::runtime_error("Not enough memory to append data to message.");
}

void Message::Writer::push_int32(std::int32_t value)
{
    if (!dbus_message_iter_append_basic(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::int32),
                std::addressof(value)))
        throw std::runtime_error("Not enough memory to append data to message.");
}

void Message::Writer::push_uint32(std::uint32_t value)
{
    if (!dbus_message_iter_append_basic(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::uint32),
                std::addressof(value)))
        throw std::runtime_error("Not enough memory to append data to message.");
}

void Message::Writer::push_int64(std::int64_t value)
{
    if (!dbus_message_iter_append_basic(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::int64),
                std::addressof(value)))
        throw std::runtime_error("Not enough memory to append data to message.");
}

void Message::Writer::push_uint64(std::uint64_t value)
{
    if (!dbus_message_iter_append_basic(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::uint64),
                std::addressof(value)))
        throw std::runtime_error("Not enough memory to append data to message.");
}

void Message::Writer::push_floating_point(double value)
{
    if (!dbus_message_iter_append_basic(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::floating_point),
                std::addressof(value)))
        throw std::runtime_error("Not enough memory to append data to message.");
}

void Message::Writer::push_stringn(const char* value, std::size_t)
{
    if (!dbus_message_iter_append_basic(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::string),
                std::addressof(value)))
        throw std::runtime_error("Not enough memory to append data to message.");
}

void Message::Writer::push_object_path(const types::ObjectPath& value)
{
    const char* s = value.as_string().c_str();
    if (!dbus_message_iter_append_basic(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::object_path),
                &s))
        throw std::runtime_error("Not enough memory to append data to message.");
}

void Message::Writer::push_signature(const types::Signature& value)
{
    const char* s = value.as_string().c_str();

    if (!dbus_message_iter_append_basic(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::signature),
                &s))
        throw std::runtime_error("Not enough memory to append data to message.");
}

void Message::Writer::push_unix_fd(const types::UnixFd& value)
{
    if (!dbus_message_iter_append_basic(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::unix_fd),
                std::addressof(value.to_int())))
        throw std::runtime_error("Not enough memory to append data to message.");
}

Message::Writer Message::Writer::open_array(const types::Signature& signature)
{
    Writer w(d->msg);
    if (!dbus_message_iter_open_container(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::array),
                signature.as_string().c_str(),
                std::addressof(w.d->iter)))
        throw std::runtime_error("Problem opening container");

    return w;
}

void Message::Writer::close_array(Message::Writer w)
{
    dbus_message_iter_close_container(
                std::addressof(d->iter),
                std::addressof(w.d->iter));
}

Message::Writer Message::Writer::open_structure()
{
    Writer w(d->msg);
    if (!dbus_message_iter_open_container(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::structure),
                nullptr,
                std::addressof(w.d->iter)))
        throw std::runtime_error("Problem opening container");

    return w;
}

void Message::Writer::close_structure(Message::Writer w)
{
    dbus_message_iter_close_container(
                std::addressof(d->iter),
                std::addressof(w.d->iter));
}

Message::Writer Message::Writer::open_variant(const types::Signature& signature)
{
    // TODO(tvoss): We really should check that the signature refers to a
    // single complete type here.

    Writer w(d->msg);
    if (!dbus_message_iter_open_container(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::variant),
                signature.as_string().c_str(),
                std::addressof(w.d->iter)))
        throw std::runtime_error("Problem opening container");

    return w;
}

void Message::Writer::close_variant(Writer w)
{
    dbus_message_iter_close_container(
                std::addressof(d->iter),
                std::addressof(w.d->iter));
}

Message::Writer Message::Writer::open_dict_entry()
{
    Writer w(d->msg);
    if (!dbus_message_iter_open_container(
                std::addressof(d->iter),
                static_cast<int>(ArgumentType::dictionary_entry),
                nullptr,
                std::addressof(w.d->iter)))
        throw std::runtime_error("Problem opening container");

    return w;
}

void Message::Writer::close_dict_entry(Message::Writer w)
{
    dbus_message_iter_close_container(
                std::addressof(d->iter),
                std::addressof(w.d->iter));
}

std::shared_ptr<Message> Message::make_method_call(
        const std::string& destination,
        const types::ObjectPath& path,
        const std::string& interface,
        const std::string& method)
{
    return std::shared_ptr<Message>(
                new Message(
                    std::unique_ptr<Message::Private>(
                        new Message::Private(
                            dbus_message_new_method_call(
                                destination.c_str(),
                                path.as_string().c_str(),
                                interface.c_str(),
                                method.c_str())))));
}

std::shared_ptr<Message> Message::make_method_return(const Message::Ptr& msg)
{
    return std::shared_ptr<Message>(
                new Message(
                    std::unique_ptr<Message::Private>(
                        new Message::Private(
                            dbus_message_new_method_return(
                                msg->d->dbus_message.get())))));
}

std::shared_ptr<Message> Message::make_signal(
        const std::string& path,
        const std::string& interface,
        const std::string& signal)
{
    return std::shared_ptr<Message>(
                new Message(
                    std::unique_ptr<Message::Private>(
                        new Message::Private(
                            dbus_message_new_signal(
                                path.c_str(),
                                interface.c_str(),
                                signal.c_str())))));
}

std::shared_ptr<Message> Message::make_error(
        const Message::Ptr& in_reply_to,
        const std::string& error_name,
        const std::string& error_desc)
{
    return std::shared_ptr<Message>(
                new Message(
                    std::unique_ptr<Message::Private>(
                        new Message::Private(
                            dbus_message_new_error(
                                in_reply_to->d->dbus_message.get(),
                                error_name.c_str(),
                                error_desc.c_str())))));
}

std::shared_ptr<Message> Message::from_raw_message(DBusMessage* msg)
{
    return std::shared_ptr<Message>(
                new Message(
                    std::unique_ptr<Message::Private>(
                        new Message::Private(
                            msg, true))));
}

Message::Type Message::type() const
{
    return static_cast<Type>(dbus_message_get_type(d->dbus_message.get()));
}

bool Message::expects_reply() const
{
    return !dbus_message_get_no_reply(d->dbus_message.get());
}

types::ObjectPath Message::path() const
{
    return types::ObjectPath(dbus_message_get_path(d->dbus_message.get()));
}

std::string Message::member() const
{
    return dbus_message_get_member(d->dbus_message.get());
}

std::string Message::signature() const
{
    return dbus_message_get_signature(d->dbus_message.get());
}

std::string Message::interface() const
{
    return dbus_message_get_interface(d->dbus_message.get());
}

std::string Message::destination() const
{
    return dbus_message_get_destination(d->dbus_message.get());
}

std::string Message::sender() const
{
    return dbus_message_get_sender(d->dbus_message.get());
}

Error Message::error() const
{
    if (type() != Message::Type::error)
        throw std::runtime_error("Message does not contain error information");

    Error result;
    dbus_set_error_from_message(std::addressof(result.raw()), d->dbus_message.get());

    return result;
}

Message::Reader Message::reader()
{
    Reader result{shared_from_this()};
    if (!dbus_message_iter_init(
                d->dbus_message.get(),
                std::addressof(result.d->iter)))
        throw std::runtime_error(
                "Could not initialize reader, message does not have arguments");
    return result;
}

Message::Writer Message::writer()
{
    Writer w(shared_from_this());

    dbus_message_iter_init_append(
                d->dbus_message.get(),
                std::addressof(w.d->iter));

    return w;
}

void Message::ensure_serial_larger_than_zero_for_testing()
{
    dbus_message_set_serial(d->dbus_message.get(), 2);
}

Message::Message(std::unique_ptr<Message::Private> p)
    : d(std::move(p))
{
    if (!d->dbus_message)
        throw std::runtime_error(
                "Precondition violated, cannot construct Message from null DBusMessage.");
}

Message::~Message()
{
}

std::shared_ptr<Message> Message::clone()
{
    return std::shared_ptr<Message>(new Message(d->clone()));
}

std::ostream& operator<<(std::ostream& out, Message::Type type)
{
    switch (type)
    {
    case Message::Type::error:
        return out << "error";
    case Message::Type::invalid:
        return out << "invalid";
    case Message::Type::method_call:
        return out << "method_call";
    case Message::Type::method_return:
        return out << "method_return";
    case Message::Type::signal:
        return out << "signal";
    }

    return out;
}
}
}
