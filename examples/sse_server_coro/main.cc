/**
 * SSE Server Coroutine Example
 *
 * This example demonstrates how to create an SSE (Server-Sent Events) endpoint
 * using Drogon's HttpResponse::newSseResponse() with C++20 coroutines.
 */

#include <drogon/drogon.h>
#include <iostream>
#include <chrono>

using namespace drogon;
using namespace std::chrono_literals;

// Coroutine-based SSE handler
Task<> sendSseEvents(SseWriterPtr writer)
{
    // Send retry interval
    co_await writer->sendCoro("retry", "3000");

    for (int i = 0; i < 10 && writer->isOpen(); ++i)
    {
        // Create event
        SseEvent event;
        event.event = "update";
        event.data = "Count: " + std::to_string(i);
        event.id = std::to_string(i);

        // Send using coroutine
        bool sent = co_await writer->sendCoro(event);
        if (!sent)
        {
            LOG_INFO << "Failed to send event, connection may be closed";
            break;
        }

        // Sleep using drogon's async sleep
        co_await drogon::sleepCoro(app().getLoop(), 1.0);
    }

    // Send completion event
    SseEvent finalEvent;
    finalEvent.event = "complete";
    finalEvent.data = "Stream finished successfully";
    co_await writer->sendCoro(finalEvent);

    writer->close();
    LOG_INFO << "Coroutine SSE stream completed";
}

int main()
{
    trantor::Logger::setLogLevel(trantor::Logger::kInfo);

    // SSE endpoint using coroutines
    app().registerHandler(
        "/sse",
        [](const HttpRequestPtr &req,
           std::function<void(const HttpResponsePtr &)> &&callback) {
            LOG_INFO << "New coroutine SSE client connected";

            auto resp = HttpResponse::newSseResponse(
                [](const SseWriterPtr &writer) {
                    // Launch the coroutine
                    async_run(sendSseEvents(writer));
                },
                true  // Disable kickoff timeout
            );

            callback(resp);
        },
        {Get});

    // JSON SSE endpoint with coroutines
    app().registerHandler(
        "/sse/json",
        [](const HttpRequestPtr &req,
           std::function<void(const HttpResponsePtr &)> &&callback) {
            LOG_INFO << "New JSON coroutine SSE client connected";

            auto resp = HttpResponse::newSseResponse(
                [](const SseWriterPtr &writer) {
                    async_run([](SseWriterPtr w) -> Task<> {
                        for (int i = 0; i < 5 && w->isOpen(); ++i)
                        {
                            Json::Value data;
                            data["timestamp"] =
                                static_cast<Json::Int64>(time(nullptr));
                            data["iteration"] = i;
                            data["status"] = "active";

                            // Send JSON using the synchronous method
                            // (sendJson doesn't have a coro version yet)
                            if (!w->sendJson(data, "data"))
                            {
                                break;
                            }

                            co_await drogon::sleepCoro(app().getLoop(), 2.0);
                        }

                        w->close();
                    }(writer));
                },
                true);

            callback(resp);
        },
        {Get});

    // Simple HTML test page
    app().registerHandler(
        "/",
        [](const HttpRequestPtr &req,
           std::function<void(const HttpResponsePtr &)> &&callback) {
            std::string html = R"(
<!DOCTYPE html>
<html>
<head><title>SSE Coroutine Demo</title></head>
<body>
    <h1>SSE Coroutine Demo</h1>
    <button onclick="start()">Start SSE</button>
    <button onclick="startJson()">Start JSON SSE</button>
    <pre id="log"></pre>
    <script>
        let es;
        function log(msg) {
            document.getElementById('log').textContent += msg + '\n';
        }
        function start() {
            if (es) es.close();
            es = new EventSource('/sse');
            es.onmessage = e => log('msg: ' + e.data);
            es.addEventListener('update', e => log('update: ' + e.data));
            es.addEventListener('complete', e => { log('complete: ' + e.data); es.close(); });
            es.onerror = () => log('error/closed');
        }
        function startJson() {
            if (es) es.close();
            es = new EventSource('/sse/json');
            es.addEventListener('data', e => log('data: ' + e.data));
            es.onerror = () => log('error/closed');
        }
    </script>
</body>
</html>
)";
            auto resp = HttpResponse::newHttpResponse();
            resp->setContentTypeCode(CT_TEXT_HTML);
            resp->setBody(html);
            callback(resp);
        },
        {Get});

    LOG_INFO << "SSE Coroutine Server running on http://127.0.0.1:8848";

    app().addListener("127.0.0.1", 8848);
    app().run();

    return 0;
}
