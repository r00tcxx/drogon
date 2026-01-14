/**
 *
 *  @file SseWriter.cc
 *
 *  Copyright 2024, An Tao.  All rights reserved.
 *  https://github.com/an-tao/drogon
 *  Use of this source code is governed by the MIT license
 *  that can be found in the License file.
 *
 *  Drogon
 *
 */

#include <drogon/HttpResponse.h>
#include <sstream>

namespace drogon
{
SseWriter::SseWriter(ResponseStreamPtr stream) : stream_(std::move(stream))
{
}

SseWriter::~SseWriter()
{
    close();
}

bool SseWriter::send(const SseEvent &event)
{
    if (!stream_ || closed_)
    {
        return false;
    }

    std::string message = formatEvent(event);
    return stream_->send(message);
}

bool SseWriter::sendJson(const Json::Value &json, const std::string &eventType)
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";  // Compact JSON
    std::string data = Json::writeString(builder, json);
    SseEvent event;
    event.event = eventType;
    event.data = std::move(data);
    return send(event);
}

bool SseWriter::sendComment(const std::string &comment)
{
    if (!stream_ || closed_)
    {
        return false;
    }

    std::string message;
    if (comment.empty())
    {
        message = ":\n\n";
    }
    else
    {
        message = ":" + comment + "\n\n";
    }
    return stream_->send(message);
}

bool SseWriter::sendRetry(int retryMs)
{
    if (!stream_ || closed_)
    {
        return false;
    }

    std::string message = "retry:" + std::to_string(retryMs) + "\n\n";
    return stream_->send(message);
}

void SseWriter::close()
{
    if (stream_ && !closed_)
    {
        closed_ = true;
        stream_->close();
        stream_.reset();
    }
}

std::string SseWriter::formatEvent(const SseEvent &event) const
{
    std::ostringstream oss;

    // Event type (if not empty)
    if (!event.event.empty())
    {
        oss << "event:" << event.event << "\n";
    }

    // Event ID
    if (!event.id.empty())
    {
        oss << "id:" << event.id << "\n";
    }

    // Retry interval
    if (event.retry > 0)
    {
        oss << "retry:" << event.retry << "\n";
    }

    // Data lines (split by newlines)
    if (!event.data.empty())
    {
        std::istringstream dataStream(event.data);
        std::string line;
        while (std::getline(dataStream, line))
        {
            oss << "data:" << line << "\n";
        }
    }
    else
    {
        // Empty data still needs a data field
        oss << "data:\n";
    }

    // Empty line to dispatch the event
    oss << "\n";

    return oss.str();
}

}  // namespace drogon
