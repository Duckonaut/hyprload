#pragma once
#include <cstddef>
#include <cstdint>
#include <variant>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef int size;
typedef size_t usize;

typedef int fd_t;
typedef int flock_t;

namespace hyprload {
    template <typename T, typename E>
    class Result {
      public:
        Result(T&& value) : m_internal(std::move(value)) {}

        Result(E&& value) : m_internal(std::move(value)) {}

        static Result<T, E> ok(T&& value) {
            return Result<T, E>(std::move(value));
        }

        static Result<T, E> err(E&& value) {
            return Result<T, E>(std::move(value));
        }

        bool isOk() const {
            return std::holds_alternative<T>(m_internal);
        }

        bool isErr() const {
            return std::holds_alternative<E>(m_internal);
        }

        T unwrap() {
            return std::get<T>(m_internal);
        }

        E unwrapErr() {
            return std::get<E>(m_internal);
        }

      private:
        std::variant<T, E> m_internal;
    };
}
