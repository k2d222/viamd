#pragma once

#include <memory>
#include <vector>

#include <rtc/websocketserver.hpp>
#include <rtc/websocket.hpp>
#include <viamd.h>

struct Api {
    rtc::WebSocketServer server;
    std::vector<std::shared_ptr<rtc::WebSocket>> clients;
};

namespace api {
    Api create(int port = 8080);
    void initialize(Api& api, ApplicationState* data);
    void update(Api& api, ApplicationState* data);
}
