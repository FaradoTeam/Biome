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

    int run();
    void stop();

private:
    bool initialize();
    void cleanup();

private:
    std::shared_ptr<RestServer> m_restServer;

    bool m_isRunning; 
};

} // namespace server
} // namespace farado
