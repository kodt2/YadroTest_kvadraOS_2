#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <deque>
#include <map>
#include <nlohmann/json.hpp>

struct SystemStats {
    double ramUsage;
    double ramTotalGB;
    double ramUsedGB;
    double ramFreeGB;
    std::vector<double> loadAvg;
    double cpuUsage;
    std::map<std::string, double> temperatures;
};

struct ProcessInfo {
    int pid;
    std::string name;
    double cpuUsage;
    double memUsageMB;
};

struct CPUData {
    unsigned long long idle;
    unsigned long long total;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ProcessInfo, pid, name, cpuUsage, memUsageMB)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SystemStats, ramUsage, ramTotalGB, ramUsedGB, ramFreeGB, loadAvg, cpuUsage, temperatures)

class SystemMonitor {
public:
    SystemMonitor();
    SystemStats getLatestStats();
    std::vector<ProcessInfo> getTopProcesses();
    std::vector<SystemStats> getHistory();
    
    void updateMetrics();
    void updateProcessList();

private:
    CPUData lastCPU {0, 0};
    std::mutex dataMutex;
    
    SystemStats currentStats;
    std::deque<SystemStats> history;
    const size_t maxHistory = 300;

    std::vector<ProcessInfo> topProcesses;

    double calculateCPU();
    std::map<std::string, double> readTemperatures();
};