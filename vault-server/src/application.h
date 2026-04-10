#pragma once

#include <memory>
#include <string>

namespace farado
{
namespace server
{

class RestServer;

class Application final
{
public:
    explicit Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    int run();
    void stop();

private:
    bool initialize();
    void cleanup();

private:
    std::unique_ptr<RestServer> m_restServer;

    bool m_isRunning { false };
};

} // namespace server
} // namespace farado
