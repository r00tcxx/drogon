/**
 * SSE Client Example
 *
 * This example demonstrates how to use the SSE (Server-Sent Events) client
 * feature of Drogon's HttpClient.
 */

#include <drogon/drogon.h>
#include <iostream>

using namespace drogon;

int main()
{
    trantor::Logger::setLogLevel(trantor::Logger::kInfo);

    // Create an HTTP client
    auto client = HttpClient::newHttpClient("http://localhost:8080");

    // Create a request for SSE endpoint
    auto req = HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/sse");

    std::cout << "Connecting to SSE endpoint..." << std::endl;

    // Send SSE request with callbacks
    client->sendRequestForSse(
        req,
        // Event callback - called for each SSE event received
        [](const SseEvent &event) {
            std::cout << "=== SSE Event ===" << std::endl;
            std::cout << "Event Type: " << event.event << std::endl;
            std::cout << "Data: " << event.data << std::endl;
            if (!event.id.empty())
            {
                std::cout << "ID: " << event.id << std::endl;
            }
            if (event.retry > 0)
            {
                std::cout << "Retry: " << event.retry << "ms" << std::endl;
            }
            std::cout << "=================" << std::endl;
        },
        // Closed callback - called when connection closes or error occurs
        [](ReqResult result, const HttpResponsePtr &resp) {
            std::cout << "SSE Connection closed with result: " << result
                      << std::endl;
            if (resp)
            {
                std::cout << "HTTP Status: " << resp->getStatusCode()
                          << std::endl;
            }
            // Stop the event loop
            app().quit();
        },
        // Headers callback (optional) - called when response headers are
        // received
        [](const HttpResponsePtr &resp) {
            std::cout << "Received headers, status: " << resp->getStatusCode()
                      << std::endl;
            auto contentType = resp->getHeader("content-type");
            std::cout << "Content-Type: " << contentType << std::endl;
        },
        30.0  // Timeout in seconds (0 = no timeout)
    );

    std::cout << "SSE request sent, waiting for events..." << std::endl;

    // Run the event loop
    app().run();

    return 0;
}
