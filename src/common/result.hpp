#pragma once

#include <cassert>
#include <variant>
#include <utility>

namespace ccl {

template <typename T, typename E>
class Result {
public:
    [[nodiscard]] static Result success(T value) {
        return Result(std::in_place_index<0>, std::move(value));
    }

    [[nodiscard]] static Result failure(E error) {
        return Result(std::in_place_index<1>, std::move(error));
    }

    [[nodiscard]] bool ok() const noexcept { return has_value(); }
    [[nodiscard]] bool has_value() const noexcept { return value_.index() == 0; }

    [[nodiscard]] T& value() & {
        assert(has_value());
        return *std::get_if<0>(&value_);
    }

    [[nodiscard]] const T& value() const& {
        assert(has_value());
        return *std::get_if<0>(&value_);
    }

    [[nodiscard]] T&& value() && {
        assert(has_value());
        return std::move(*std::get_if<0>(&value_));
    }

    [[nodiscard]] E& error() & {
        assert(!has_value());
        return *std::get_if<1>(&value_);
    }

    [[nodiscard]] const E& error() const& {
        assert(!has_value());
        return *std::get_if<1>(&value_);
    }

private:
    template <std::size_t Index, typename Value>
    explicit Result(std::in_place_index_t<Index> index, Value&& value)
        : value_(index, std::forward<Value>(value)) {}

    std::variant<T, E> value_;
};

} // namespace ccl
