/**
 *
 *  @file SseEvent.cc
 *
 *  Copyright 2024, An Tao.  All rights reserved.
 *  https://github.com/an-tao/drogon
 *  Use of this source code is governed by the MIT license
 *  that can be found in the License file.
 *
 *  Drogon
 *
 */

#include <drogon/SseEvent.h>

namespace drogon
{
SseEventPtr SseEvent::newEvent()
{
    return std::make_shared<SseEvent>();
}

SseEventPtr SseEvent::newEvent(const std::string &data)
{
    auto event = std::make_shared<SseEvent>();
    event->setData(data);
    return event;
}

SseEventPtr SseEvent::newEvent(const std::string &eventType,
                               const std::string &data)
{
    auto event = std::make_shared<SseEvent>();
    event->setEvent(eventType);
    event->setData(data);
    return event;
}

}  // namespace drogon
