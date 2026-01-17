/**
 *
 *  @file SseEventParser.cc
 *
 *  Copyright 2024, An Tao.  All rights reserved.
 *  https://github.com/an-tao/drogon
 *  Use of this source code is governed by the MIT license
 *  that can be found in the License file.
 *
 *  Drogon
 *
 */

#include "SseEventParser.h"
#include <trantor/utils/Logger.h>
#include <algorithm>
#include <cstdlib>

namespace drogon
{
bool SseEventParser::parse(trantor::MsgBuffer *buf)
{
    while (buf->readableBytes() > 0)
    {
        // Find end of line (LF or CRLF)
        const char *data = buf->peek();
        const char *end = data + buf->readableBytes();
        const char *lineEnd = nullptr;

        for (const char *p = data; p < end; ++p)
        {
            if (*p == '\n')
            {
                lineEnd = p;
                break;
            }
        }

        if (!lineEnd)
        {
            // No complete line yet, wait for more data
            break;
        }

        // Extract the line (excluding LF, and optionally CR before LF)
        size_t lineLen = lineEnd - data;
        if (lineLen > 0 && data[lineLen - 1] == '\r')
        {
            --lineLen;
        }

        std::string line(data, lineLen);
        buf->retrieve(lineEnd - data + 1);  // +1 for LF

        processLine(line);
    }

    return true;
}

void SseEventParser::processLine(const std::string &line)
{
    // Empty line means dispatch the current event
    if (line.empty())
    {
        dispatchEvent();
        return;
    }

    // Lines starting with colon are comments, ignore them
    if (line[0] == ':')
    {
        return;
    }

    // Find the field name and value
    std::string field;
    std::string value;

    auto colonPos = line.find(':');
    if (colonPos == std::string::npos)
    {
        // No colon, the entire line is the field name with empty value
        field = line;
    }
    else
    {
        field = line.substr(0, colonPos);
        // Skip the colon and optional space after it
        size_t valueStart = colonPos + 1;
        if (valueStart < line.size() && line[valueStart] == ' ')
        {
            ++valueStart;
        }
        value = line.substr(valueStart);
    }

    // Process the field
    if (field == "event")
    {
        currentEvent_->setEvent(value);
    }
    else if (field == "data")
    {
        // Multiple data fields are concatenated with newlines
        if (!currentEvent_->data().empty())
        {
            std::string newData = currentEvent_->data() + '\n' + value;
            currentEvent_->setData(std::move(newData));
        }
        else
        {
            currentEvent_->setData(value);
        }
    }
    else if (field == "id")
    {
        // ID must not contain null characters
        if (value.find('\0') == std::string::npos)
        {
            currentEvent_->setId(value);
            lastEventId_ = value;
        }
    }
    else if (field == "retry")
    {
        // Retry value must be all digits
        bool allDigits = !value.empty() &&
                         std::all_of(value.begin(), value.end(), ::isdigit);
        if (allDigits)
        {
            currentEvent_->setRetry(std::atoi(value.c_str()));
        }
    }
    // Ignore unknown fields
}

void SseEventParser::dispatchEvent()
{
    if (currentEvent_->isValid())
    {
        // If no event type specified, use "message"
        if (currentEvent_->event().empty())
        {
            currentEvent_->setEvent("message");
        }

        if (eventCallback_)
        {
            eventCallback_(currentEvent_);
        }
    }

    currentEvent_ = SseEvent::newEvent();
}

}  // namespace drogon
