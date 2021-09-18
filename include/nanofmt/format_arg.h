// Copyright (c) Sean Middleditch and contributors. See accompanying LICENSE.md for copyright details.

#pragma once

#include "config.h"

#include <type_traits>

namespace NANOFMT_NS::detail {
    // avoid explicitly pulling in <memory>
    template <typename T>
    constexpr T* addressof(T const& value) noexcept {
        return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char&>(value)));
    }

    // avoid explicitly pulling in <utility>
    template <typename T>
    T&& declval() noexcept;

    // helper for c++17 and older
    template <class T>
    using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
} // namespace NANOFMT_NS::detail

namespace NANOFMT_NS {
    /// Supported core format_value types
    enum class format_value_type {
        t_mono,
        t_int,
        t_uint,
        t_long,
        t_ulong,
        t_longlong,
        t_ulonglong,
        t_char,
        t_float,
        t_double,
        t_bool,
        t_cstring,
        t_voidptr,
        t_custom
    };

    struct format_value {
        struct custom {
            void (*thunk)(void const* value, char const** spec, char const* end, buffer& buf) = nullptr;
            void const* value = nullptr;
        };

        constexpr format_value() noexcept : v_int(0) {}
        constexpr format_value(int value) noexcept : v_int(value), type(format_value_type::t_int) {}
        constexpr format_value(unsigned value) noexcept : v_unsigned(value), type(format_value_type::t_uint) {}
        constexpr format_value(long value) noexcept : v_long(value), type(format_value_type::t_long) {}
        constexpr format_value(unsigned long value) noexcept : v_ulong(value), type(format_value_type::t_ulong) {}
        constexpr format_value(long long value) noexcept : v_longlong(value), type(format_value_type::t_longlong) {}
        constexpr format_value(unsigned long long value) noexcept
            : v_ulonglong(value)
            , type(format_value_type::t_ulonglong) {}
        constexpr format_value(char value) noexcept : v_char(value), type(format_value_type::t_char) {}
        constexpr format_value(float value) noexcept : v_float(value), type(format_value_type::t_float) {}
        constexpr format_value(double value) noexcept : v_double(value), type(format_value_type::t_double) {}
        constexpr format_value(bool value) noexcept : v_bool(value), type(format_value_type::t_bool) {}
        constexpr format_value(char const* value) noexcept : v_cstring(value), type(format_value_type::t_cstring) {}
        constexpr format_value(void const* value) noexcept : v_voidptr(value), type(format_value_type::t_voidptr) {}
        constexpr format_value(custom value) noexcept : v_custom(value), type(format_value_type::t_custom) {}

        union {
            int v_int;
            unsigned v_unsigned;
            long v_long;
            unsigned long v_ulong;
            long long v_longlong;
            unsigned long long v_ulonglong;
            char v_char;
            float v_float;
            double v_double;
            bool v_bool;
            char const* v_cstring;
            void const* v_voidptr;
            custom v_custom;
        };

        format_value_type type = format_value_type::t_mono;
    };
} // namespace NANOFMT_NS

namespace NANOFMT_NS::detail {
    template <typename T>
    struct value_type_map {
        using type = T;
    };
    template <>
    struct value_type_map<signed char> {
        using type = signed int;
    };
    template <>
    struct value_type_map<unsigned char> {
        using type = signed int;
    };
    template <>
    struct value_type_map<signed short> {
        using type = signed int;
    };
    template <>
    struct value_type_map<unsigned short> {
        using type = signed int;
    };
    template <>
    struct value_type_map<decltype(nullptr)> {
        using type = void const*;
    };
    template <typename T>
    using value_type_map_t = typename value_type_map<T>::type;

    template <size_t N>
    struct value_store {
        static constexpr size_t size = N;
        format_value values[N + 1 /* avoid size 0 */];
    };

    template <typename T, typename = void>
    struct has_formatter {
        static constexpr bool value = false;
    };
    template <typename T>
    struct has_formatter<
        T,
        std::void_t<
            decltype(declval<formatter<T>>().parse(declval<char const*>(), declval<char const*>())),
            decltype(declval<formatter<T>>().format(declval<T>(), declval<buffer&>()))>> {
        static constexpr bool value = true;
    };

    template <typename ValueT, typename FormatterT = formatter<ValueT>>
    void formatter_thunk(void const* value, char const** in, char const* end, buffer& buf) {
        FormatterT fmt;
        if (in != nullptr) {
            *in = fmt.parse(*in, end);
        }
        fmt.format(*static_cast<ValueT const*>(value), buf);
    }

    template <typename ValueT>
    constexpr format_value make_format_value(ValueT const& value) noexcept {
        using T = std::decay_t<remove_cvref_t<ValueT>>;
        if constexpr (std::is_constructible_v<format_value, value_type_map_t<T>>) {
            return value_type_map_t<T>(value);
        }
        else if constexpr (has_formatter<T>::value) {
            format_value::custom custom;
            custom.thunk = &formatter_thunk<T>;
            custom.value = addressof(value);
            return custom;
        }
        else if constexpr (std::is_enum_v<T>) {
            return static_cast<value_type_map_t<std::underlying_type_t<T>>>(value);
        }
    };
} // namespace NANOFMT_NS::detail

namespace NANOFMT_NS {
    /// List of format args.
    ///
    /// Only use this type as a temporary value!
    ///
    struct format_args {
        template <size_t N>
        constexpr /*implicit*/ format_args(detail::value_store<N>&& values) noexcept
            : values(values.values)
            , count(N) {}

        void format(unsigned index, char const** in, char const* end, buffer& buf) const;

        format_value const* values = nullptr;
        size_t count = 0;
    };

    template <typename... Args>
    constexpr auto make_format_args(Args const&... args) noexcept {
        return detail::value_store<sizeof...(Args)>{detail::make_format_value(args)...};
    }
} // namespace NANOFMT_NS
