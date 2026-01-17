/**
 *
 *  @file SseEvent.h
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
#include <drogon/HttpTypes.h>
#include <string>
#include <memory>
#include <functional>

namespace drogon
{
class HttpResponse;
using HttpResponsePtr = std::shared_ptr<HttpResponse>;

class SseEvent;
using SseEventPtr = std::shared_ptr<SseEvent>;

/**
 * @brief Represents a Server-Sent Event (SSE) message
 *
 * SSE events follow the format:
 *   event: eventName
 *   id: eventId
 *   retry: retryMs
 *   data: eventData
 *
 */
class DROGON_EXPORT SseEvent
{
  public:
    /**
     * @brief Create a new SSE event
     * @return A shared pointer to the new event
     */
    static SseEventPtr newEvent();

    /**
     * @brief Create a new SSE event with data
     * @param data The event data
     * @return A shared pointer to the new event
     */
    static SseEventPtr newEvent(const std::string &data);

    /**
     * @brief Create a new SSE event with event type and data
     * @param eventType The event type
     * @param data The event data
     * @return A shared pointer to the new event
     */
    static SseEventPtr newEvent(const std::string &eventType,
                                const std::string &data);

    /// The event type. Empty string means "message" event.
    const std::string &event() const
    {
        return event_;
    }

    void setEvent(const std::string &event)
    {
        event_ = event;
    }

    void setEvent(std::string &&event)
    {
        event_ = std::move(event);
    }

    /// The event data. Multiple data lines are concatenated with newlines.
    const std::string &data() const
    {
        return data_;
    }

    void setData(const std::string &data)
    {
        data_ = data;
    }

    void setData(std::string &&data)
    {
        data_ = std::move(data);
    }

    /// The event ID. Used for reconnection.
    const std::string &id() const
    {
        return id_;
    }

    void setId(const std::string &id)
    {
        id_ = id;
    }

    void setId(std::string &&id)
    {
        id_ = std::move(id);
    }

    /// Retry time in milliseconds. 0 means not specified.
    int retry() const
    {
        return retry_;
    }

    void setRetry(int retry)
    {
        retry_ = retry;
    }

    /// Check if the event is valid (has data)
    bool isValid() const
    {
        return !data_.empty();
    }

    /// Reset the event to empty state
    void reset()
    {
        event_.clear();
        data_.clear();
        id_.clear();
        retry_ = 0;
    }

  private:
    std::string event_;
    std::string data_;
    std::string id_;
    int retry_{0};
};

/// Callback for receiving SSE events
using SseEventCallback = std::function<void(const SseEventPtr &)>;

/// Callback for SSE connection closed or error
using SseClosedCallback = std::function<void(ReqResult, const HttpResponsePtr &)>;

/// Callback for SSE headers received (optional, for checking response status)
using SseHeadersCallback = std::function<void(const HttpResponsePtr &)>;

}  // namespace drogon
