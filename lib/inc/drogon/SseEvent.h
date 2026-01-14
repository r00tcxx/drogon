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
struct DROGON_EXPORT SseEvent
{
    /// The event type. Empty string means "message" event.
    std::string event;

    /// The event data. Multiple data lines are concatenated with newlines.
    std::string data;

    /// The event ID. Used for reconnection.
    std::string id;

    /// Retry time in milliseconds. 0 means not specified.
    int retry{0};

    /// Check if the event is valid (has data)
    bool isValid() const
    {
        return !data.empty();
    }

    /// Reset the event to empty state
    void reset()
    {
        event.clear();
        data.clear();
        id.clear();
        retry = 0;
    }
};

/// Callback for receiving SSE events
using SseEventCallback = std::function<void(const SseEvent &)>;

/// Callback for SSE connection closed or error
using SseClosedCallback = std::function<void(ReqResult, const HttpResponsePtr &)>;

/// Callback for SSE headers received (optional, for checking response status)
using SseHeadersCallback = std::function<void(const HttpResponsePtr &)>;

}  // namespace drogon
