#include "api.h"

#include <core/md_log.h>
#include <viamd.h>

#include <nlohmann/json.hpp>
using rtc::WebSocket;
using json = nlohmann::json;

Api api::create(int port) {
    return Api {
        .server = rtc::WebSocketServer({ .port = port })
    };
}

void api::initialize(Api &api, ApplicationState& state) {
    api.clients.clear();

    api.server.onClient([&](std::shared_ptr<rtc::WebSocket> client) {
        api.clients.push_back(client);
        std::string addr = client->remoteAddress().value_or("unknown address");
        VIAMD_LOG_INFO("WebSocket client connected: %s", addr.c_str());
        client->onMessage([&](rtc::message_variant msg) {
            std::string text = std::get<std::string>(msg);
            VIAMD_LOG_DEBUG("client message: %s", text.c_str());
            state.editor.SetText(text);
            json data = json::parse(text);
        });
        client->onClosed([&]() {
             VIAMD_LOG_INFO("WebSocket client disconnected: %s", addr.c_str());
         });
    });
}

void api::update(Api &api, ApplicationState &state) {
    if (state.editor.IsTextChanged()) {
        for (auto& client : api.clients) {
            client->send(state.editor.GetText());
        }
    }
}
