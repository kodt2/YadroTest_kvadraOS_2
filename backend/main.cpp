#include <iostream>
#include <vector>
#include <set>
#include <mutex>
#include <thread>
#include <chrono>
#include <ixwebsocket/IXWebSocketServer.h>
#include "httplib.h"
#include <nlohmann/json.hpp>
#include "SystemMonitor.hpp"

using json = nlohmann::json;

std::set<std::shared_ptr<ix::WebSocket>> clients;
std::mutex clientsMutex;

void broadcastMetrics(const std::string& data) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto it = clients.begin(); it != clients.end(); ) {
        auto& client = *it;
        if (client->getReadyState() == ix::ReadyState::Open) {
            client->send(data);
            ++it;
        } else {
            it = clients.erase(it);
        }
    }
}

int main() {
    httplib::Server svr;
    SystemMonitor monitor;

    svr.set_mount_point("/", "./public");

    ix::WebSocketServer wsServer(8081, "0.0.0.0");

    wsServer.setOnConnectionCallback([&](std::weak_ptr<ix::WebSocket> webSocketPtr, std::shared_ptr<ix::ConnectionState> connectionState) {
        auto webSocket = webSocketPtr.lock(); // Превращаем в shared_ptr
        if (!webSocket) return;

        webSocket->setOnMessageCallback([webSocketPtr](const ix::WebSocketMessagePtr& msg) {
        auto ws = webSocketPtr.lock();
        if (!ws) return;

        if (msg->type == ix::WebSocketMessageType::Open) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.insert(ws);
        } else if (msg->type == ix::WebSocketMessageType::Close) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.erase(ws);
        }
    });
    });

    auto res = wsServer.listen();
    if (!res.first) {
        std::cerr << "WS Server error: " << res.second << std::endl;
        return 1;
    }
    wsServer.start();

    std::thread httpThread([&]() {
        
        svr.Get("/api/stats", [&](const httplib::Request&, httplib::Response& res) {
            auto stats = monitor.getLatestStats();
            json j = {
                {"cpu", stats.cpuUsage},
                {"ram_usage_perc", stats.ramUsage},
                {"ram_used_gb", stats.ramUsedGB},
                {"load_avg", stats.loadAvg}
            };
            res.set_content(j.dump(), "application/json");
            res.set_header("Access-Control-Allow-Origin", "*");
        });

        std::cout << "HTTP Server started at http://localhost:8080" << std::endl;
        svr.listen("0.0.0.0", 8080);
    });

    std::cout << "WebSocket Server started at ws://localhost:8081" << std::endl;
    
    while (true) {
        monitor.updateMetrics();
        monitor.updateProcessList();

        auto stats = monitor.getLatestStats();
        auto procs = monitor.getTopProcesses();
        
        json j;
        j["cpu"] = stats.cpuUsage;
        j["ram_usage_perc"] = stats.ramUsage;
        j["ram_used_gb"] = stats.ramTotalGB - stats.ramFreeGB;
        j["ram_total_gb"] = stats.ramTotalGB;
        j["load_avg"] = stats.loadAvg;

        j["temps"] = stats.temperatures;

        json procList = json::array();
        for (const auto& p : procs) {
            procList.push_back({
                {"pid", p.pid},
                {"name", p.name},
                {"memUsageMB", p.memUsageMB}
            });
        }
        j["procs"] = procList;

        broadcastMetrics(j.dump());
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    if (httpThread.joinable()) httpThread.join();
    return 0;
}