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
#ifndef CORE_DBUS_PROPERTY_H_
#define CORE_DBUS_PROPERTY_H_

#include <core/dbus/types/any.h>
#include <core/dbus/types/variant.h>

#include <core/property.h>

#include <list>
#include <memory>

namespace core
{
namespace dbus
{
class Object;

/**
 * @brief Models a DBus property.
 * @tparam PropertyType Underlying value type of the property.
 */
template<typename PropertyType>
class Property : public core::Property<typename PropertyType::ValueType>
{
public:
    typedef typename PropertyType::ValueType ValueType;
    typedef core::Property<ValueType> Super;

    inline ~Property();

    /**
     * @brief Non-mutable access to the contained value.
     * @return Non-mutable reference to the contained value.
     */
    inline const ValueType& get() const;

    /**
     * @brief Adjusts the contained value
     * @param [in] new_value New value of the property.
     */
    inline void set(const ValueType& new_value);

    /**
     * @brief Queries whether the property is writable.
     * @return true if the property is writable, false otherwise.
     */
    inline bool is_writable() const;

    /**
     * @brief Emitted during destruction of an object instance.
     */
    inline const core::Signal<void>& about_to_be_destroyed() const;

protected:
    friend class Object;

    inline static std::shared_ptr<Property<PropertyType>> make_property(
        const std::shared_ptr<Object>& parent);

private:
    inline Property(
        const std::shared_ptr<Object>& parent,
        const std::string& interface,
        const std::string& name,
        bool writable);

    inline void handle_get(const Message::Ptr& msg);
    inline void handle_set(const Message::Ptr& msg);
    inline void handle_changed(const types::Variant& msg);

    std::shared_ptr<Object> parent;
    std::string interface;
    std::string name;
    bool writable;
    core::Signal<void> signal_about_to_be_destroyed;
};
}
}

#include "impl/property.h"

#endif // CORE_DBUS_PROPERTY_H_
