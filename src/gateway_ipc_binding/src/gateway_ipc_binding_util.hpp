/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#ifndef SRC_GATEWAY_IPC_BINDING_SRC_GATEWAY_IPC_BINDING_UTIL
#define SRC_GATEWAY_IPC_BINDING_SRC_GATEWAY_IPC_BINDING_UTIL

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <score/gateway_ipc_binding/gateway_ipc_binding.hpp>
#include <thread>

namespace score::gateway_ipc_binding {

/// \brief Message frame header for all IPC messages
struct Message_frame_header {
    Message_type type;
    std::uint16_t payload_size;
};

// \brief IPC message with type and payload
template <typename T>
struct Message_frame {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    Message_frame_header header{T::type, sizeof(T)};
    T payload;
};

inline Message_type get_message_type(std::uint8_t data) noexcept {
    static_assert(sizeof(Message_type) == sizeof(std::uint8_t), "Message_type must be 1 byte");
    return static_cast<Message_type>(data);
}

/// \brief Check if the incoming message matches the expected type and can be safely cast to
/// Msg_type
/// \tparam Msg_type Expected message type to check and cast to
/// \param data Incoming message data (including framing)
/// \return Pointer to the cast message if type and size are valid, std::nullopt otherwise
template <typename Msg_type>
std::optional<Msg_type const*> check_and_cast(score::cpp::span<std::uint8_t const> data) noexcept {
    static_assert(std::is_trivially_copyable_v<Message_frame<Msg_type>>,
                  "Message_frame<Msg_type> must be trivially copyable");

    if (data.empty()) {
        return std::nullopt;  // Invalid frame - too small
    }

    auto const type = get_message_type(data[0]);

    if (type != Msg_type::type) {
        return std::nullopt;  // We only handle the specified message type at this scope
    }

    if (data.size() < sizeof(Message_frame<Msg_type>)) {
        return std::nullopt;  // Invalid frame - insufficient data
    }

    Message_frame<Msg_type> const* const connect =
        reinterpret_cast<Message_frame<Msg_type> const*>(data.data());

    if (connect->header.payload_size != sizeof(Msg_type)) {
        return std::nullopt;  // Invalid payload size
    }

    return &connect->payload;
}

struct Service_counts {
    std::uint16_t num_methods{0U};
    std::uint16_t num_events{0U};
};

template <typename T>
class Id_generator {
    T m_next_id;

   public:
    Id_generator(T start = T{}) : m_next_id(std::move(start)) {}

    T get_next_id() noexcept { return m_next_id++; }
};

class Lock_guard {
    std::optional<std::lock_guard<std::mutex>> m_guard;
    std::atomic<std::optional<std::thread::id>>* m_thread_id = nullptr;

   public:
    Lock_guard(std::mutex& mutex, std::atomic<std::optional<std::thread::id>>& thread_id)
        : m_thread_id(&thread_id) {
        auto const current_thread_id = m_thread_id->load();

        if (!current_thread_id || (*current_thread_id != std::this_thread::get_id())) {
            m_guard.emplace(mutex);
            *m_thread_id = std::this_thread::get_id();
        }
    }

    Lock_guard(Lock_guard const&) = delete;
    Lock_guard(Lock_guard&&) = delete;

    ~Lock_guard() { unlock(); }

    Lock_guard& operator=(Lock_guard const&) = delete;
    Lock_guard& operator=(Lock_guard&&) = delete;

    void unlock() {
        if (!m_guard) {
            return;
        }
        *m_thread_id = std::nullopt;
        m_guard.reset();
    }
};

/// Callback_aware_mutex does not lock a second time, if this thread already holds the mutex
///
/// Using this mutexes do not have to be released when calling callbacks into user code, which might
/// trigger more actions.
class Callback_aware_mutex {
    std::mutex m_mutex;
    std::atomic<std::optional<std::thread::id>> m_thread_id;

   public:
    Lock_guard lock() { return Lock_guard{m_mutex, m_thread_id}; }
};

}  // namespace score::gateway_ipc_binding

#endif  // SRC_GATEWAY_IPC_BINDING_SRC_GATEWAY_IPC_BINDING_UTIL
