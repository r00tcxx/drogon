/**
 * SSE Client Coroutine Example
 *
 * This example demonstrates how to use the SSE (Server-Sent Events) client
 * feature with C++20 coroutines.
 */

#include <drogon/drogon.h>
#include <iostream>

using namespace drogon;

// Coroutine function to handle SSE connection
Task<> handleSse()
{
    auto client = HttpClient::newHttpClient("http://localhost:8080");

    auto req = HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/sse");

    std::cout << "Connecting to SSE endpoint with coroutines..." << std::endl;

    try
    {
        // The coroutine will suspend until the SSE connection closes
        // Events are delivered via the callback during the connection
        auto resp = co_await client->sendRequestForSseCoro(
            req,
            [](const SseEventPtr &event) {
                std::cout << "[" << event->event() << "] " << event->data()
                          << std::endl;
            },
            30.0  // Timeout
        );

        std::cout << "SSE connection completed normally" << std::endl;
        if (resp)
        {
            std::cout << "Final status: " << resp->getStatusCode() << std::endl;
        }
    }
    catch (const HttpException &e)
    {
        std::cout << "SSE connection error: " << e.what() << std::endl;
    }

    app().quit();
    co_return;
}

int main()
{
    trantor::Logger::setLogLevel(trantor::Logger::kInfo);

    // Start the SSE handler coroutine
    async_run(handleSse());

    // Run the event loop
    app().run();

    return 0;
}
