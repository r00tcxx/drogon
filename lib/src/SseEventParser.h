/**
 *
 *  @file SseEventParser.h
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

#include <drogon/SseEvent.h>
#include <trantor/utils/MsgBuffer.h>
#include <trantor/utils/NonCopyable.h>
#include <string>
#include <functional>

namespace drogon
{
/**
 * @brief Parser for Server-Sent Events (SSE) stream
 *
 * Parses the SSE text/event-stream format:
 *   event: <event-type>
 *   id: <event-id>
 *   retry: <retry-time-ms>
 *   data: <event-data>
 *   <blank line>
 */
class SseEventParser : public trantor::NonCopyable
{
  public:
    using EventCallback = std::function<void(const SseEventPtr &)>;

    explicit SseEventParser(EventCallback callback)
        : eventCallback_(std::move(callback))
    {
    }

    /**
     * @brief Parse data from buffer and emit events
     *
     * @param buf The message buffer containing SSE data
     * @return true if parsing was successful
     */
    bool parse(trantor::MsgBuffer *buf);

    /**
     * @brief Reset the parser state
     */
    void reset()
    {
        currentEvent_ = SseEvent::newEvent();
        lineBuffer_.clear();
    }

    /**
     * @brief Get the last event ID received
     */
    const std::string &lastEventId() const
    {
        return lastEventId_;
    }

  private:
    /**
     * @brief Process a single line of SSE data
     */
    void processLine(const std::string &line);

    /**
     * @brief Dispatch the current event if valid
     */
    void dispatchEvent();

    EventCallback eventCallback_;
    SseEventPtr currentEvent_{SseEvent::newEvent()};
    std::string lineBuffer_;
    std::string lastEventId_;
};

}  // namespace drogon
