/**
 * SSE Server Example
 *
 * This example demonstrates how to create an SSE (Server-Sent Events) endpoint
 * using Drogon's HttpResponse::newSseResponse().
 */

#include <drogon/drogon.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace drogon;
using namespace std::chrono_literals;

// Shared counter for demo purposes
std::atomic<int> eventCounter{0};

int main()
{
    trantor::Logger::setLogLevel(trantor::Logger::kInfo);

    // Simple SSE endpoint - sends events periodically
    app().registerHandler(
        "/sse",
        [](const HttpRequestPtr &req,
           std::function<void(const HttpResponsePtr &)> &&callback) {
            LOG_INFO << "New SSE client connected";

            auto resp = HttpResponse::newSseResponse(
                [](const SseWriterPtr &writer) {
                    // Run event sending in a separate thread
                    std::thread([writer]() {
                        int count = 0;
                        while (count < 10 && writer->isOpen())
                        {
                            // Send a simple message
                            writer->send("Hello from SSE server! Count: " +
                                         std::to_string(count));

                            std::this_thread::sleep_for(1s);
                            count++;
                        }

                        // Send a final event with custom type
                        auto finalEvent = SseEvent::newEvent();
                        finalEvent->setEvent("complete");
                        finalEvent->setData("Stream finished");
                        finalEvent->setId("final");
                        writer->send(finalEvent);

                        // Close the connection
                        writer->close();
                        LOG_INFO << "SSE stream completed";
                    }).detach();
                },
                true  // Disable kickoff timeout for long-lived connection
            );

            callback(resp);
        },
        {Get});

    // SSE endpoint with JSON data
    app().registerHandler(
        "/sse/json",
        [](const HttpRequestPtr &req,
           std::function<void(const HttpResponsePtr &)> &&callback) {
            LOG_INFO << "New JSON SSE client connected";

            auto resp = HttpResponse::newSseResponse(
                [](const SseWriterPtr &writer) {
                    std::thread([writer]() {
                        for (int i = 0; i < 5 && writer->isOpen(); ++i)
                        {
                            Json::Value data;
                            data["timestamp"] =
                                static_cast<Json::Int64>(time(nullptr));
                            data["counter"] = ++eventCounter;
                            data["message"] = "JSON event data";

                            writer->sendJson(data, "data-update");

                            std::this_thread::sleep_for(2s);
                        }

                        writer->close();
                    }).detach();
                });

            callback(resp);
        },
        {Get});

    // SSE endpoint demonstrating all features
    app().registerHandler(
        "/sse/demo",
        [](const HttpRequestPtr &req,
           std::function<void(const HttpResponsePtr &)> &&callback) {
            LOG_INFO << "New demo SSE client connected";

            auto resp = HttpResponse::newSseResponse(
                [](const SseWriterPtr &writer) {
                    std::thread([writer]() {
                        // Send retry interval
                        writer->sendRetry(3000);  // 3 seconds

                        // Send a comment (keep-alive)
                        writer->sendComment("Connection established");

                        // Send event with all fields
                        auto event = SseEvent::newEvent();
                        event->setEvent("init");
                        event->setData("Initialization complete");
                        event->setId("1");
                        writer->send(event);

                        std::this_thread::sleep_for(1s);

                        // Send multi-line data
                        event->setEvent("multi-line");
                        event->setData("Line 1\nLine 2\nLine 3");
                        event->setId("2");
                        writer->send(event);

                        std::this_thread::sleep_for(1s);

                        // Keep-alive comments
                        for (int i = 0; i < 3 && writer->isOpen(); ++i)
                        {
                            writer->sendComment();  // Empty comment as heartbeat
                            std::this_thread::sleep_for(500ms);
                        }

                        // Final message
                        writer->send("done", "Demo complete!");
                        writer->close();
                    }).detach();
                });

            callback(resp);
        },
        {Get});

    // HTML page to test SSE
    app().registerHandler(
        "/",
        [](const HttpRequestPtr &req,
           std::function<void(const HttpResponsePtr &)> &&callback) {
            std::string html = R"html(
<!DOCTYPE html>
<html>
<head>
    <title>SSE Demo</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .event { padding: 10px; margin: 5px 0; background: #f0f0f0; border-radius: 4px; }
        .error { background: #ffcccc; }
        button { margin: 10px 5px; padding: 10px 20px; }
    </style>
</head>
<body>
    <h1>Server-Sent Events Demo</h1>
    
    <button onclick="connect('/sse')">Connect to /sse</button>
    <button onclick="connect('/sse/json')">Connect to /sse/json</button>
    <button onclick="connect('/sse/demo')">Connect to /sse/demo</button>
    <button onclick="disconnect()">Disconnect</button>
    
    <h2>Events:</h2>
    <div id="events"></div>
    
    <script>
        let eventSource = null;
        
        function connect(url) {
            disconnect();
            
            const events = document.getElementById('events');
            events.innerHTML = '<div class="event">Connecting to ' + url + '...</div>';
            
            eventSource = new EventSource(url);
            
            eventSource.onopen = function(e) {
                addEvent('Connected!', 'open');
            };
            
            eventSource.onmessage = function(e) {
                addEvent(e.data, 'message', e.lastEventId);
            };
            
            eventSource.onerror = function(e) {
                addEvent('Connection error or closed', 'error');
            };
            
            // Custom event handlers
            ['init', 'complete', 'data-update', 'multi-line', 'done'].forEach(type => {
                eventSource.addEventListener(type, function(e) {
                    addEvent(e.data, type, e.lastEventId);
                });
            });
        }
        
        function disconnect() {
            if (eventSource) {
                eventSource.close();
                eventSource = null;
                addEvent('Disconnected', 'info');
            }
        }
        
        function addEvent(data, type, id) {
            const events = document.getElementById('events');
            const div = document.createElement('div');
            div.className = 'event' + (type === 'error' ? ' error' : '');
            let text = '<strong>[' + type + ']</strong> ' + data;
            if (id) text += ' (id: ' + id + ')';
            div.innerHTML = text;
            events.insertBefore(div, events.firstChild);
        }
    </script>
</body>
</html>
)html";

            auto resp = HttpResponse::newHttpResponse();
            resp->setContentTypeCode(CT_TEXT_HTML);
            resp->setBody(html);
            callback(resp);
        },
        {Get});

    LOG_INFO << "SSE Server running on http://127.0.0.1:8848";
    LOG_INFO << "Open http://127.0.0.1:8848/ in your browser to test";

    app().addListener("127.0.0.1", 8848);
    app().run();

    return 0;
}
