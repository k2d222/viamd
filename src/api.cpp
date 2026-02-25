#include "api.h"

#include <algorithm>
#include <string>
#include <vector>

#include <core/md_log.h>
#include <core/md_str.h>
#include <core/md_vec_math.h>
#include <viamd.h>

#include <nlohmann/json.hpp>
using rtc::WebSocket;
using json = nlohmann::json;

Api api::create(int port) {
    return Api {
        .server = rtc::WebSocketServer({ .port = port })
    };
}

void extract_json_vec3(vec3_t& v, const json& json) {
    v.x = json[0];
    v.y = json[1];
    v.z = json[2];
}

void extract_json_quat(quat_t& q, const json& json) {
    q.x = json[0];
    q.y = json[1];
    q.z = json[2];
    q.w = json[3];
}

RepresentationType extract_rep_type(const std::string& str) {
    auto it = std::ranges::find(representation_type_str, str.c_str());
    return (RepresentationType)(it - representation_type_str);
}

ColorMapping extract_color_mapping(const std::string& str) {
    auto it = std::ranges::find(color_mapping_str, str.c_str());
    return (ColorMapping)(it - color_mapping_str);
}

// this is roughly the same as viamd.cpp::load_workspace, but less feature-rich.
void load_json_state(ApplicationState* data, std::string text) {
    json j = json::parse(text);

    if (j.contains("editor")) {
        data->editor.SetText(j["editor"]);
    }

    if (j.contains("camera")) {
        auto const& c = j["camera"];
        extract_json_vec3(data->view.camera.position, c["position"]);
        extract_json_quat(data->view.camera.orientation, c["orientation"]);
        data->view.camera.focus_distance = c["distance"];
    }

    if (j.contains("representations")) {
        remove_all_representations(data);
        for (auto const& r : j["representations"]) {
            Representation* rep = create_representation(data);
            std::string name = r["name"];
            std::string type = r["type"];
            std::string mapping = r["mapping"];
            std::string filt = r["filter"];
            str_copy_to_char_buf(rep->name, sizeof(rep->name), str_t{ name.data(), name.size() });
            rep->type = extract_rep_type(type);
            rep->color_mapping = extract_color_mapping(mapping);
            str_copy_to_char_buf(rep->filt, sizeof(rep->filt), str_t{ filt.data(), filt.size() });
            
        }
    }
}

void api::initialize(Api& api, ApplicationState* data) {
    api.clients.clear();

    api.server.onClient([&](std::shared_ptr<rtc::WebSocket> client) {
        api.clients.push_back(client);
        std::string addr = client->remoteAddress().value_or("unknown address");
        VIAMD_LOG_INFO("WebSocket client connected: %s", addr.c_str());

        client->onMessage([&](rtc::message_variant msg) {
            std::string text = std::get<std::string>(msg);

            try {
                load_json_state(data, text);
            } catch (...) {
                VIAMD_LOG_ERROR("invalid client message: %s", text.c_str());
                return;
            }
        });

        client->onClosed([&]() {
             VIAMD_LOG_INFO("WebSocket client disconnected: %s", addr.c_str());
             auto it = std::ranges::find(api.clients, client);
             if (it != api.clients.end()) api.clients.erase(it);
         });
    });
}

void api::update(Api& api, ApplicationState* data) {
    if (data->editor.IsTextChanged()) {
        for (auto& client : api.clients) {
            json j = json{
                { "editor", data->editor.GetText() }
            };
            client->send(j.dump());
        }
    }
}
