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

#include <atomic>
#include <cassert>
#include <memory>
#include <score/gateway_ipc_binding/error.hpp>
#include <score/gateway_ipc_binding/gateway_ipc_binding.hpp>
#include <score/gateway_ipc_binding/gateway_ipc_binding_client.hpp>
#include <score/socom/runtime.hpp>
#include <thread>
#include <utility>

#include "binding_base.hpp"
#include "reply_channel.hpp"

namespace score::gateway_ipc_binding {

namespace {

/// \brief Implementation of Gateway_ipc_binding_client
class Gateway_ipc_binding_client_impl : public Gateway_ipc_binding_client, public Reply_channel {
   public:
    /// \brief Constructor
    /// \param runtime SOCom runtime instance
    /// \param channel Unique pointer to bidirectional channel client
    /// \param slot_manager Unique pointer to shared memory slot manager
    /// \param find_service_elements Service elements to advertise
    Gateway_ipc_binding_client_impl(
        score::socom::Runtime& runtime,
        score::cpp::pmr::unique_ptr<score::message_passing::IClientConnection> channel,
        Shared_memory_manager_factory::Sptr slot_manager,
        Find_service_elements find_service_elements)
        : m_binding_base{runtime, std::move(slot_manager)},
          m_channel(std::move(channel)),
          m_find_service_elements(std::move(find_service_elements)) {
        m_channel->Start([this](auto const state) { on_state_change(state); },
                         [this](auto const data) { on_receive_message(data); });
    }

    ~Gateway_ipc_binding_client_impl() {
        // ensure no callbacks are in-flight that might access member variables after they've been
        // destroyed
        m_channel->Stop();
        while (m_channel->GetState() !=
               score::message_passing::IClientConnection::State::kStopped) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    Result<void> send(score::cpp::span<std::uint8_t const> data) noexcept override {
        if (m_channel == nullptr) {
            return MakeUnexpected(Bidirectional_channel_error::runtime_error_send_failed,
                                  "Channel is not available");
        }

        auto const send_result = m_channel->Send(data);
        if (!send_result) {
            return MakeUnexpected(Bidirectional_channel_error::runtime_error_send_failed,
                                  "Failed to send message: " + send_result.error().ToString());
        }
        return {};
    }

    using Reply_channel::send;  // Bring template send() into scope

    bool is_connected() const noexcept override { return m_connected; }

    void on_state_change(score::message_passing::IClientConnection::State const state) {
        switch (state) {
            case score::message_passing::IClientConnection::State::kReady: {
                Message_frame<Connect> connect_msg;
                connect_msg.payload.find_service_elements = m_find_service_elements;
                auto status = send(connect_msg);
                assert(status);

                break;
            }
            case score::message_passing::IClientConnection::State::kStopped:
                m_connected = false;
                m_binding_base.remove_client(client_id);
                break;
            case score::message_passing::IClientConnection::State::kStarting:
            case score::message_passing::IClientConnection::State::kStopping:
            default:
                m_connected = false;
                break;
        }
    }

    void on_receive_message(score::cpp::span<std::uint8_t const> data) noexcept {
        if (data.empty() || m_channel == nullptr) {
            return;  // Ignore empty messages or when destructor runs
        }

        auto message_type = get_message_type(data[0]);

        if (Message_type::Connect_reply == message_type) {
            auto msg_opt = check_and_cast<Connect_reply>(data);
            if (!msg_opt) {
                // Invalid message - log and ignore
                return;
            }

            handle_connect_reply_message(**msg_opt);
        }
        m_binding_base.on_receive_message(client_id, *this, data);
    }

    void handle_connect_reply_message(Connect_reply const& msg) {
        assert(msg.status);
        m_connected = msg.status;

        if (m_connected) {
            m_binding_base.add_client(client_id, *this);
        }
    }

   private:
    std::atomic<bool> m_connected{false};
    // There is only ever one connection: from this instance to the server
    constexpr static Client_id client_id{0};
    Gateway_ipc_binding_base m_binding_base;
    score::cpp::pmr::unique_ptr<score::message_passing::IClientConnection> m_channel;
    Find_service_elements m_find_service_elements;
};

}  // namespace

Result<std::unique_ptr<Gateway_ipc_binding_client>> Gateway_ipc_binding_client::create(
    score::socom::Runtime& runtime,
    score::cpp::pmr::unique_ptr<score::message_passing::IClientConnection> connection,
    Shared_memory_manager_factory::Uptr slot_manager,
    Find_service_elements find_service_elements) noexcept {
    if (!connection) {
        return MakeUnexpected(Bidirectional_channel_error::runtime_error_not_connected,
                              "Client connection is not available");
    }

    return std::make_unique<Gateway_ipc_binding_client_impl>(
        runtime, std::move(connection), std::move(slot_manager), std::move(find_service_elements));
}

}  // namespace score::gateway_ipc_binding
