/**
 *
 *  @file SseClientContext.cc
 *
 *  Copyright 2024, An Tao.  All rights reserved.
 *  https://github.com/an-tao/drogon
 *  Use of this source code is governed by the MIT license
 *  that can be found in the License file.
 *
 *  Drogon
 *
 */

#include "SseClientContext.h"
#include <trantor/utils/Logger.h>
#include <algorithm>

// Avoid min/max macro issues on Windows
#ifdef _WIN32
#undef min
#undef max
#endif

namespace drogon
{
bool SseClientContext::parse(trantor::MsgBuffer *buf)
{
    if (status_ == Status::Closed)
    {
        return false;
    }

    if (status_ == Status::ExpectHeaders)
    {
        if (!parseHeaders(buf))
        {
            return false;
        }
    }

    if (status_ == Status::ExpectBody)
    {
        if (!parseBody(buf))
        {
            return false;
        }
    }

    return true;
}

bool SseClientContext::parseHeaders(trantor::MsgBuffer *buf)
{
    if (!responsePtr_)
    {
        responsePtr_ = std::make_shared<HttpResponseImpl>();
    }

    bool hasMore = true;
    bool statusLineParsed = (responsePtr_->statusCode() != kUnknown);

    while (hasMore && buf->readableBytes() > 0)
    {
        const char *crlf = buf->findCRLF();
        if (!crlf)
        {
            hasMore = false;
            break;
        }

        if (!statusLineParsed)
        {
            // Parse status line
            if (!processResponseLine(buf->peek(), crlf))
            {
                return false;
            }
            statusLineParsed = true;
            buf->retrieveUntil(crlf + 2);
            continue;
        }

        // Parse headers
        if (crlf == buf->peek())
        {
            // Empty line - end of headers
            buf->retrieveUntil(crlf + 2);

            // Check transfer encoding
            const std::string &encoding =
                responsePtr_->getHeaderBy("transfer-encoding");
            if (encoding == "chunked")
            {
                isChunked_ = true;
                expectChunkLength_ = true;
            }
            else
            {
                const std::string &lenStr =
                    responsePtr_->getHeaderBy("content-length");
                if (!lenStr.empty())
                {
                    contentLength_ =
                        static_cast<size_t>(std::stoull(lenStr));
                }
            }

            // Initialize SSE parser
            eventParser_ = std::make_unique<SseEventParser>(
                [this](const SseEventPtr &event) {
                    if (eventCallback_ && !timedOut_)
                    {
                        eventCallback_(event);
                    }
                });

            // Call headers callback
            if (headersCallback_)
            {
                headersCallback_(responsePtr_);
            }

            status_ = Status::ExpectBody;
            hasMore = false;
        }
        else
        {
            // Parse header line
            const char *colon = std::find(buf->peek(), crlf, ':');
            if (colon != crlf)
            {
                std::string field(buf->peek(), colon);
                std::transform(field.begin(),
                               field.end(),
                               field.begin(),
                               [](unsigned char c) { return tolower(c); });
                ++colon;
                while (colon < crlf && *colon == ' ')
                {
                    ++colon;
                }
                std::string value(colon, crlf);
                responsePtr_->addHeader(std::move(field), std::move(value));
            }
            buf->retrieveUntil(crlf + 2);
        }
    }

    return true;
}

bool SseClientContext::processResponseLine(const char *begin, const char *end)
{
    const char *start = begin;
    const char *space = std::find(start, end, ' ');
    if (space != end)
    {
        if (*(space - 1) == '1')
        {
            responsePtr_->setVersion(Version::kHttp11);
        }
        else if (*(space - 1) == '0')
        {
            responsePtr_->setVersion(Version::kHttp10);
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    start = space + 1;
    space = std::find(start, end, ' ');
    if (space != end)
    {
        std::string statusCode(start, space - start);
        auto code = std::atoi(statusCode.c_str());
        responsePtr_->setStatusCode(HttpStatusCode(code));
        return true;
    }
    return false;
}

bool SseClientContext::parseBody(trantor::MsgBuffer *buf)
{
    if (!eventParser_)
    {
        return false;
    }

    if (isChunked_)
    {
        // Chunked transfer encoding
        while (buf->readableBytes() > 0)
        {
            if (expectChunkLength_)
            {
                const char *crlf = buf->findCRLF();
                if (!crlf)
                {
                    break;
                }

                std::string lenStr(buf->peek(), crlf - buf->peek());
                char *endPtr;
                currentChunkLength_ = strtol(lenStr.c_str(), &endPtr, 16);
                buf->retrieveUntil(crlf + 2);

                if (currentChunkLength_ == 0)
                {
                    // Last chunk - wait for trailing CRLF
                    crlf = buf->findCRLF();
                    if (crlf)
                    {
                        buf->retrieveUntil(crlf + 2);
                    }
                    status_ = Status::Closed;
                    return true;
                }

                expectChunkLength_ = false;
            }
            else
            {
                // Read chunk data
                size_t toRead =
                    std::min(currentChunkLength_, buf->readableBytes());
                if (toRead > 0)
                {
                    // Create a temporary buffer for SSE parsing
                    trantor::MsgBuffer tempBuf;
                    tempBuf.append(buf->peek(), toRead);
                    eventParser_->parse(&tempBuf);
                    buf->retrieve(toRead);
                    currentChunkLength_ -= toRead;
                }

                if (currentChunkLength_ == 0)
                {
                    // Chunk complete, expect CRLF
                    if (buf->readableBytes() >= 2)
                    {
                        buf->retrieve(2);  // CRLF
                        expectChunkLength_ = true;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }
    else if (contentLength_ > 0)
    {
        // Fixed content length
        size_t remaining = contentLength_ - bytesRead_;
        size_t toRead = std::min(remaining, buf->readableBytes());
        if (toRead > 0)
        {
            eventParser_->parse(buf);
            bytesRead_ += toRead;
        }

        if (bytesRead_ >= contentLength_)
        {
            status_ = Status::Closed;
        }
    }
    else
    {
        // No content-length, read until connection close
        eventParser_->parse(buf);
    }

    return true;
}

void SseClientContext::onClose(ReqResult result)
{
    if (closedCallbackInvoked_)
    {
        return;
    }
    closedCallbackInvoked_ = true;
    status_ = Status::Closed;

    if (closedCallback_)
    {
        closedCallback_(result, responsePtr_);
    }
}

}  // namespace drogon
