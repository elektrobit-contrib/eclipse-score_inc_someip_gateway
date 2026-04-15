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

#ifndef SRC_GATEWAY_IPC_BINDING_SRC_SHARED_MEMORY_PAYLOAD
#define SRC_GATEWAY_IPC_BINDING_SRC_SHARED_MEMORY_PAYLOAD

#include <cassert>
#include <score/gateway_ipc_binding/shared_memory_slot_manager.hpp>
#include <score/socom/payload.hpp>
#include <utility>

namespace score::gateway_ipc_binding {

/// \brief Writable_payload implementation backed by a shared memory slot
class Shared_memory_payload final : public score::socom::Writable_payload {
   public:
    explicit Shared_memory_payload(Shared_memory_slot_guard guard) noexcept
        : m_guard(std::move(guard)) {}

    ~Shared_memory_payload() noexcept override = default;

    Shared_memory_payload(Shared_memory_payload const&) = delete;
    Shared_memory_payload& operator=(Shared_memory_payload const&) = delete;
    Shared_memory_payload(Shared_memory_payload&&) = delete;
    Shared_memory_payload& operator=(Shared_memory_payload&&) = delete;

    [[nodiscard]] Span data() const noexcept override {
        auto const mem = m_guard.get_memory();
        return Span{mem.data(), static_cast<Span::size_type>(mem.size())};
    }

    [[nodiscard]] Span header() const noexcept override { return {}; }

    [[nodiscard]] Writable_span header() noexcept override { return {}; }

    [[nodiscard]] Writable_span wdata() noexcept override {
        auto mem = m_guard.get_memory();
        return Writable_span{mem.data(), static_cast<Writable_span::size_type>(mem.size())};
    }

    [[nodiscard]] std::size_t get_slot_handle() const noexcept override {
        assert(m_guard.get_handle());
        return *m_guard.get_handle();
    }

   private:
    Shared_memory_slot_guard m_guard;
};

}  // namespace score::gateway_ipc_binding

#endif  // SRC_GATEWAY_IPC_BINDING_SRC_SHARED_MEMORY_PAYLOAD
