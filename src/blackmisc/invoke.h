/* Copyright (C) 2015
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_INVOKE_H
#define BLACKMISC_INVOKE_H

#include <tuple>
#include "blackmisc/typetraits.h"
#include "blackmisc/integersequence.h"
#include <QtGlobal>

namespace BlackMisc
{
    //! \cond PRIVATE
    namespace Private
    {

        // Our own version of C++17 std::invoke().
        template <typename F, typename T, typename = std::enable_if_t<std::is_member_object_pointer<F>::value>>
        decltype(auto) invoke(F ptr, T && object)
        {
            return std::forward<T>(object).*ptr;
        }
        template <typename F, typename T, typename... Ts, typename = std::enable_if_t<std::is_member_function_pointer<F>::value>>
        decltype(auto) invoke(F ptr, T && object, Ts && ... args)
        {
            return (std::forward<T>(object).*ptr)(std::forward<Ts>(args)...);
        }
        template < typename F, typename... Ts, typename = std::enable_if_t < ! std::is_member_pointer<std::decay_t<F>>::value >>
        decltype(auto) invoke(F && func, Ts && ... args)
        {
            return std::forward<F>(func)(std::forward<Ts>(args)...);
        }

        // Like invoke() but ignores the first argument if callable is not a member function. For uniform calling of callables with slot semantics.
        template <typename F, typename T, typename U, size_t... Is>
        decltype(auto) invokeSlotImpl(F ptr, T *object, U tuple, std::index_sequence<Is...>, std::true_type)
        {
            Q_UNUSED(tuple); // in case the pack expansion is empty
            return (object->*ptr)(std::forward<std::tuple_element_t<Is, U>>(std::get<Is>(tuple))...);
        }
        template <typename F, typename T, typename U, size_t... Is>
        decltype(auto) invokeSlotImpl(F &&func, T *, U tuple, std::index_sequence<Is...>, std::false_type)
        {
            Q_UNUSED(tuple); // in case the pack expansion is empty
            return std::forward<F>(func)(std::forward<std::tuple_element_t<Is, U>>(std::get<Is>(tuple))...);
        }
        template <typename F, typename T, typename... Ts>
        decltype(auto) invokeSlot(F &&func, T *object, Ts &&... args)
        {
            using seq = MaskSequence < std::make_index_sequence<sizeof...(Ts)>, ! TIsQPrivateSignal<std::decay_t<Ts>>::value... >;
            return invokeSlotImpl(std::forward<F>(func), object, std::forward_as_tuple(std::forward<Ts>(args)...), seq(), std::is_member_pointer<std::decay_t<F>>());
        }

    }
    //! \endcond
}

#endif
