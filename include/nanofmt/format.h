// Copyright (c) Sean Middleditch and contributors. See accompanying LICENSE.md for copyright details.

#pragma once

#include "buffer.h"
#include "config.h"

#include <cstddef>

namespace NANOFMT_NS {
    struct format_args;
    struct format_spec;
    struct format_string;
    struct format_string_view;

    /// Specialize to implement format support for a type.
    ///
    /// Two member functions must be defined:
    ///
    /// constexpr char const* parse(char const* in, char const* end) noexcept;
    ///
    /// void format(T const& value, buffer& buf);
    template <typename T>
    struct formatter;

    template <typename StringT>
    constexpr format_string to_format_string(StringT const& value) noexcept;

    /// Wrapper for format strings.
    struct format_string {
        constexpr format_string() noexcept = default;
        constexpr format_string(char const* string, std::size_t length) noexcept
            : begin(string)
            , end(string + length) {}
        template <std::size_t N>
        constexpr format_string(char const (&str)[N]) noexcept : begin(str)
                                                               , end(begin + __builtin_strlen(begin)) {}
        constexpr format_string(char const* const zstr) noexcept : begin(zstr), end(begin + __builtin_strlen(begin)) {}

        template <typename StringT>
        constexpr format_string(StringT const& string) noexcept : format_string(to_format_string(string)) {}

        char const* begin = nullptr;
        char const* end = nullptr;
    };

    /// Small wrapper to assist in formatting types like std::string_view
    struct format_string_view {
        char const* string = nullptr;
        std::size_t length = 0;
    };

    template <typename... Args>
    char* format_to(buffer& buf, format_string format_str, Args const&... args);

    inline char* vformat_to(buffer& buf, format_string format_str, format_args&& args);

    template <typename... Args>
    char* format_to_n(char* dest, std::size_t count, format_string format_str, Args const&... args);

    inline char* vformat_to_n(char* dest, std::size_t count, format_string format_str, format_args&& args);

    template <std::size_t N, typename... Args>
    char* format_to(char (&dest)[N], format_string format_str, Args const&... args);

    template <std::size_t N>
    char* vformat_to(char (&dest)[N], format_string format_str, format_args&& args);

    template <typename... Args>
    std::size_t format_size(format_string format_str, Args const&... args);

    inline std::size_t vformat_size(format_string format_str, format_args&& args);

    template <typename ValueT>
    char* format_value_to(buffer& buf, ValueT const& value, format_string spec = format_string{});

    template <typename ValueT, std::size_t N>
    char* format_value_to(char (&dest)[N], ValueT const& value, format_string spec = format_string{});

    template <typename ValueT>
    char* format_value_to_n(char* dest, std::size_t count, ValueT const& value, format_string spec = format_string{});

    template <typename ValueT>
    std::size_t format_value_size(ValueT const& value, format_string spec = format_string{});

    template <typename... Args>
    constexpr auto make_format_args(Args const&... args) noexcept;

    constexpr char const* parse_spec(
        char const* in,
        char const* end,
        format_spec& spec,
        char const* allowed_types = nullptr) noexcept;

    namespace detail {
        char* vformat(buffer& buf, format_string format_str, format_args&& args);
        constexpr int parse_nonnegative(char const*& start, char const* end) noexcept;
    } // namespace detail

    /// Default formatter spec options
    struct format_spec {
        int width = -1;
        int precision = -1;
        signed char align = -1; // -1 left, 0 center, +1 right
        char sign = '-'; // -, +, or space
        char fill = ' ';
        char type = '\0';
        bool zero_pad = false;
        bool alt_form = false;
        bool locale = false;
    };

    namespace detail {
        template <typename T>
        struct default_formatter {
            format_spec spec;
            char const* parse(char const* in, char const* end) noexcept;
            void format(T value, buffer& buf) noexcept;
        };

        struct char_buf_formatter {
            std::size_t max_length = 0;
            format_spec spec;
            char const* parse(char const* in, char const* end) noexcept;
            void format(char const* value, buffer& buf) noexcept;
        };
    } // namespace detail

    template <>
    struct formatter<char> : detail::default_formatter<char> {};
    template <>
    struct formatter<bool> : detail::default_formatter<bool> {};

    template <>
    struct formatter<char*> : detail::default_formatter<char const*> {};
    template <>
    struct formatter<char const*> : detail::default_formatter<char const*> {};
    template <std::size_t N>
    struct formatter<char const[N]> : detail::char_buf_formatter {};
    template <>
    struct formatter<format_string_view> : detail::default_formatter<format_string_view> {};

    template <>
    struct formatter<signed char> : detail::default_formatter<signed int> {};
    template <>
    struct formatter<unsigned char> : detail::default_formatter<unsigned int> {};
    template <>
    struct formatter<signed short> : detail::default_formatter<signed int> {};
    template <>
    struct formatter<unsigned short> : detail::default_formatter<unsigned int> {};
    template <>
    struct formatter<signed int> : detail::default_formatter<signed int> {};
    template <>
    struct formatter<unsigned int> : detail::default_formatter<unsigned int> {};
    template <>
    struct formatter<signed long> : detail::default_formatter<signed long> {};
    template <>
    struct formatter<unsigned long> : detail::default_formatter<unsigned long> {};
    template <>
    struct formatter<signed long long> : detail::default_formatter<signed long long> {};
    template <>
    struct formatter<unsigned long long> : detail::default_formatter<unsigned long long> {};

    template <>
    struct formatter<float> : detail::default_formatter<float> {};
    template <>
    struct formatter<double> : detail::default_formatter<double> {};

    template <>
    struct formatter<decltype(nullptr)> : detail::default_formatter<void const*> {};
    template <>
    struct formatter<void*> : detail::default_formatter<void const*> {};
    template <>
    struct formatter<void const*> : detail::default_formatter<void const*> {};

    /// Specialize to customize the conversion of a string type to a format_string
    template <typename StringT>
    constexpr format_string to_format_string(StringT const& value) noexcept {
        return {value.data(), value.size()};
    }

    char* vformat_to(buffer& buf, format_string format_str, format_args&& args) {
        return detail::vformat(buf, format_str, static_cast<format_args&&>(args));
    }

    char* vformat_to_n(char* dest, std::size_t count, format_string format_str, format_args&& args) {
        buffer buf(dest, count);
        return detail::vformat(buf, format_str, static_cast<format_args&&>(args));
    }

    /// Formats a string and arguments into dest, writing no more than count
    /// bytes. The output will be NUL-terminated. Returns a pointer to the
    /// last character written, which will be the NUL byte itself.
    template <typename... Args>
    char* format_to_n(char* dest, std::size_t count, format_string format_str, Args const&... args) {
        buffer buf(dest, count);
        return detail::vformat(buf, format_str, make_format_args(args...));
    }

    template <typename... Args>
    char* format_to(buffer& buf, format_string format_str, Args const&... args) {
        return detail::vformat(buf, format_str, make_format_args(args...));
    }

    /// Formats a string and arguments into dest, writing no more than N
    /// bytes. The output will be NUL-terminated. Returns a pointer to the
    /// last character written, which will be the NUL byte itself.
    template <std::size_t N, typename... Args>
    char* format_to(char (&dest)[N], format_string format_str, Args const&... args) {
        buffer buf(dest, N - 1 /*NUL*/);
        char* const end = detail::vformat(buf, format_str, make_format_args(args...));
        *end = '\0';
        return end;
    }

    /// Returns the number of characters that would be written to a
    /// destination buffer (_excluding_ any terminating NUL) for the
    /// given format string and arguments
    template <typename... Args>
    std::size_t format_size(format_string format_str, Args const&... args) {
        buffer buf(nullptr, 0);
        detail::vformat(buf, format_str, make_format_args(args...));
        return buf.advance;
    }

    std::size_t vformat_size(format_string format_str, format_args&& args) {
        buffer buf(nullptr, 0);
        detail::vformat(buf, format_str, static_cast<format_args&&>(args));
        return buf.advance;
    }

    template <typename ValueT>
    char* format_value_to(buffer& buf, ValueT const& value, format_string spec) {
        formatter<ValueT> fmt;
        if (char const* const end = fmt.parse(spec.begin, spec.end); end != spec.end) {
            return buf.pos;
        }
        fmt.format(value, buf);
        return buf.pos;
    }

    /// Formats a value into dest, writing no more than count bytes. The output will
    /// be NUL-terminated. Returns a pointer to the last character written, which
    /// will be the NUL byte itself.
    template <typename ValueT>
    char* format_value_to_n(char* dest, std::size_t count, ValueT const& value, format_string spec) {
        buffer buf(dest, count);
        return format_value_to(buf, value, spec);
    }

    /// Formats a value into dest, writing no more than N bytes. The output will
    /// be NUL-terminated. Returns a pointer to the last character written, which
    /// will be the NUL byte itself.
    template <typename ValueT, std::size_t N>
    char* format_value_to(char (&dest)[N], ValueT const& value, format_string spec) {
        buffer buf(dest, N - 1 /*NUL*/);
        char* const end = format_value_to(buf, value, spec);
        *end = '\0';
        return end;
    }

    /// Calculates the length of the buffer required to hold the formatted value,
    /// excluded the trailing NUL byte.
    template <typename ValueT>
    std::size_t format_value_size(ValueT const& value, format_string spec) {
        buffer buf(nullptr, 0);
        format_value_to(buf, value, spec);
        return buf.advance;
    }
} // namespace NANOFMT_NS

#include "format_arg.h"

namespace NANOFMT_NS {
    constexpr int detail::parse_nonnegative(char const*& start, char const* end) noexcept {
        if (start == end) {
            return -1;
        }

        if (*start == '0') {
            ++start;
            return 0;
        }

        // there must be at least one non-zero digit
        if (!(*start >= '1' && *start <= '9')) {
            return -1;
        }

        int result = 0;
        while (start != end && *start >= '0' && *start <= '9') {
            result *= 10;
            result += *start - '0';
            ++start;
        }
        return result;
    }

    /// Parses standard format specification options into the provided
    /// spec object. Returns a pointer to the next unconsumed character
    /// from the input range.
    constexpr char const* parse_spec(
        char const* in,
        char const* end,
        format_spec& spec,
        char const* allowed_types) noexcept {
        if (in == end) {
            return in;
        }

        auto const is_align = [](char const* c, char const* e) noexcept {
            return c != e && (*c == '<' || *c == '>' || *c == '^');
        };

        // -- parse fill -
        //
        // fixme: fill should be any character except { and } but only when followed
        //  by an alignment
        //
        if (*in != '{' && *in != '}' && is_align(in + 1, end)) {
            spec.fill = *in;
            ++in;

            if (in == end) {
                return in;
            }
        }

        // -- parse alignment --
        //
        switch (*in) {
            case '<':
                spec.align = -1;
                ++in;
                break;
            case '>':
                spec.align = +1;
                ++in;
                break;
            case '^':
                spec.align = 0;
                ++in;
                break;
            default:
                break;
        }
        if (in == end) {
            return in;
        }

        // -- parse sign --
        //
        switch (*in) {
            case '+':
            case '-':
            case ' ':
                spec.sign = *in++;
                if (in == end) {
                    return in;
                }
                break;
            default:
                break;
        }

        // -- parse alternate form flag --
        //
        if (*in == '#') {
            spec.alt_form = true;
            ++in;
            if (in == end) {
                return in;
            }
        }

        // -- parse zero pad flag --
        //
        if (*in == '0') {
            spec.zero_pad = true;
            ++in;
            if (in == end) {
                return in;
            }
        }

        // -- parse width
        //
        if (int const width = detail::parse_nonnegative(in, end); width >= 0) {
            spec.width = width;
            if (in == end) {
                return in;
            }

            // a width of 0 is not allowed
            if (width == 0) {
                return --in;
            }
        }

        // -- parse precision
        //
        if (*in == '.') {
            ++in;
            int const precision = detail::parse_nonnegative(in, end);

            if (precision < 0 || in == end) {
                return in;
            }

            spec.precision = precision;
        }

        // -- parse locale flag
        //
        if (*in == 'L') {
            spec.locale = true;
            ++in;
            if (in == end) {
                return in;
            }
        }

        // -- parse type
        //
        if (allowed_types != nullptr) {
            for (char const* t = allowed_types; *t != 0; ++t) {
                if (*in == *t) {
                    spec.type = *in++;
                    break;
                }
            }
        }

        return in;
    }
} // namespace NANOFMT_NS
