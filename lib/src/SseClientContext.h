/**
 *
 *  @file SseClientContext.h
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

#include "SseEventParser.h"
#include "HttpResponseImpl.h"
#include <drogon/SseEvent.h>
#include <trantor/net/TcpConnection.h>
#include <trantor/utils/MsgBuffer.h>
#include <trantor/utils/NonCopyable.h>
#include <memory>
#include <functional>
#include <atomic>

namespace drogon
{
/**
 * @brief Context for SSE client connection
 *
 * This class manages the state of an SSE connection including:
 * - HTTP response header parsing
 * - SSE event parsing
 * - Timeout management
 * - Callback invocation
 */
class SseClientContext : public trantor::NonCopyable,
                         public std::enable_shared_from_this<SseClientContext>
{
  public:
    enum class Status
    {
        ExpectHeaders,
        ExpectBody,
        Closed
    };

    SseClientContext(SseEventCallback eventCb,
                     SseClosedCallback closedCb,
                     SseHeadersCallback headersCb)
        : eventCallback_(std::move(eventCb)),
          closedCallback_(std::move(closedCb)),
          headersCallback_(std::move(headersCb)),
          status_(Status::ExpectHeaders)
    {
    }

    /**
     * @brief Parse incoming data
     * @return true if parsing should continue, false on error
     */
    bool parse(trantor::MsgBuffer *buf);

    /**
     * @brief Called when connection is closed
     */
    void onClose(ReqResult result);

    /**
     * @brief Get current status
     */
    Status status() const
    {
        return status_;
    }

    /**
     * @brief Check if headers have been received
     */
    bool headersReceived() const
    {
        return status_ != Status::ExpectHeaders;
    }

    /**
     * @brief Get the response (for headers)
     */
    const HttpResponseImplPtr &response() const
    {
        return responsePtr_;
    }

    /**
     * @brief Set timeout flag
     */
    void setTimedOut()
    {
        timedOut_ = true;
    }

    /**
     * @brief Check if timed out
     */
    bool isTimedOut() const
    {
        return timedOut_;
    }

    /**
     * @brief Mark as closed
     */
    void setClosed()
    {
        status_ = Status::Closed;
    }

  private:
    bool parseHeaders(trantor::MsgBuffer *buf);
    bool parseBody(trantor::MsgBuffer *buf);
    bool processResponseLine(const char *begin, const char *end);

    SseEventCallback eventCallback_;
    SseClosedCallback closedCallback_;
    SseHeadersCallback headersCallback_;

    HttpResponseImplPtr responsePtr_;
    std::unique_ptr<SseEventParser> eventParser_;

    Status status_;
    std::atomic<bool> timedOut_{false};
    bool closedCallbackInvoked_{false};

    // For chunked transfer encoding
    bool isChunked_{false};
    size_t currentChunkLength_{0};
    bool expectChunkLength_{false};
    size_t contentLength_{0};
    size_t bytesRead_{0};
};

using SseClientContextPtr = std::shared_ptr<SseClientContext>;

}  // namespace drogon
