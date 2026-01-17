/**
 *
 *  @file SseWriter.h
 *
 *  Copyright 2024, An Tao.  All rights reserved.
 *  https://github.com/an-tao/drogon
 *  Use of this source code is governed by the MIT license
 *  that can be found in the License file.
 *
 *  Drogon
 *
 */
#pragma once

#include <drogon/exports.h>
#include <drogon/SseEvent.h>
#include <trantor/net/AsyncStream.h>
#include <json/json.h>
#include <memory>
#include <string>
#include <string_view>
#include <sstream>
#include <atomic>

#ifdef __cpp_impl_coroutine
#include <drogon/utils/coroutine.h>
#endif

namespace drogon
{
// Forward declare ResponseStream
class ResponseStream;
using ResponseStreamPtr = std::unique_ptr<ResponseStream>;

/**
 * @brief A writer for sending Server-Sent Events (SSE) to clients.
 *
 * This class provides a convenient interface for sending SSE messages
 * from a server to connected clients. It handles the proper formatting
 * of SSE messages including event types, data, IDs, and retry intervals.
 *
 * Usage example:
 * @code
 * auto resp = HttpResponse::newSseResponse(
 *     [](const SseWriterPtr &writer) {
 *         writer->send("Hello, World!");
 *         writer->send(SseEvent{.event = "update", .data = "{\"count\": 1}"});
 *         writer->close();
 *     });
 * callback(resp);
 * @endcode
 */
class DROGON_EXPORT SseWriter
{
  public:
    /**
     * @brief Construct a new SseWriter with the underlying response stream
     *
     * @param stream The response stream for sending data (takes ownership)
     */
    explicit SseWriter(ResponseStreamPtr stream);

    ~SseWriter();

    // Non-copyable
    SseWriter(const SseWriter &) = delete;
    SseWriter &operator=(const SseWriter &) = delete;

    /**
     * @brief Send an SSE event
     *
     * @param event The SSE event to send
     * @return true if the event was sent successfully
     * @return false if the connection is closed or an error occurred
     */
    bool send(const SseEventPtr &event);

    /**
     * @brief Send a simple data message with default event type "message"
     *
     * @param data The data to send
     * @return true if the message was sent successfully
     * @return false if the connection is closed
     */
    bool send(const std::string &data)
    {
        auto event = SseEvent::newEvent(data);
        return send(event);
    }

    /**
     * @brief Send an event with a specific event type and data
     *
     * @param eventType The event type name
     * @param data The event data
     * @return true if the message was sent successfully
     * @return false if the connection is closed
     */
    bool send(const std::string &eventType, const std::string &data)
    {
        auto event = SseEvent::newEvent(eventType, data);
        return send(event);
    }

    /**
     * @brief Send a JSON object as SSE data
     *
     * @param json The JSON value to send
     * @param eventType Optional event type (defaults to "message")
     * @return true if the message was sent successfully
     * @return false if the connection is closed
     */
    bool sendJson(const Json::Value &json,
                  const std::string &eventType = std::string());

    /**
     * @brief Send a comment (for keep-alive purposes)
     *
     * SSE comments start with a colon and are ignored by clients.
     * Useful for keeping the connection alive.
     *
     * @param comment The comment text (without leading colon)
     * @return true if sent successfully
     */
    bool sendComment(const std::string &comment = std::string());

    /**
     * @brief Send a retry interval instruction
     *
     * Tells the client how long to wait before reconnecting after a
     * connection is lost.
     *
     * @param retryMs Retry interval in milliseconds
     * @return true if sent successfully
     */
    bool sendRetry(int retryMs);

    /**
     * @brief Close the SSE connection
     *
     * After calling this method, no more events can be sent.
     */
    void close();

    /**
     * @brief Check if the connection is still open
     *
     * @return true if the connection is open
     * @return false if the connection has been closed
     */
    bool isOpen() const
    {
        return stream_ && !closed_;
    }

#ifdef __cpp_impl_coroutine
    /**
     * @brief Coroutine-friendly send that can be awaited
     *
     * @param event The SSE event to send
     * @return An awaitable that resolves to true if sent successfully
     */
    Task<bool> sendCoro(const SseEventPtr &event)
    {
        co_return send(event);
    }

    /**
     * @brief Coroutine-friendly send for simple data
     *
     * @param data The data to send
     * @return An awaitable that resolves to true if sent successfully
     */
    Task<bool> sendCoro(const std::string &data)
    {
        co_return send(data);
    }

    /**
     * @brief Coroutine-friendly send with event type
     *
     * @param eventType The event type name
     * @param data The event data
     * @return An awaitable that resolves to true if sent successfully
     */
    Task<bool> sendCoro(const std::string &eventType, const std::string &data)
    {
        co_return send(eventType, data);
    }
#endif

  private:
    /**
     * @brief Format an SSE event according to the specification
     */
    std::string formatEvent(const SseEventPtr &event) const;

    ResponseStreamPtr stream_;
    std::atomic<bool> closed_{false};
};

using SseWriterPtr = std::shared_ptr<SseWriter>;

/// Callback type for SSE response handlers
using SseWriterCallback = std::function<void(const SseWriterPtr &)>;

}  // namespace drogon
