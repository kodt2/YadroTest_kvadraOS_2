#include "SystemMonitor.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <dirent.h>
#include <unistd.h>

SystemMonitor::SystemMonitor() {
    calculateCPU(); 
}

void SystemMonitor::updateMetrics() {
    SystemStats stats;
    
    std::ifstream memFile("/proc/meminfo");
    std::string line, key;
    long long value;
    std::map<std::string, long long> memData;
    while (std::getline(memFile, line)) {
        std::replace(line.begin(), line.end(), ':', ' ');
        std::stringstream ss(line);
        if (ss >> key >> value) memData[key] = value;
    }

    double total = static_cast<double>(memData["MemTotal"]);
    double available = static_cast<double>(memData["MemAvailable"]);
    
    stats.ramTotalGB = total / (1024.0 * 1024.0);
    stats.ramFreeGB = available / (1024.0 * 1024.0);
    stats.ramUsedGB = stats.ramTotalGB - stats.ramFreeGB;
    stats.ramUsage = (1.0 - available / total) * 100.0;

    std::ifstream laFile("/proc/loadavg");
    double l1, l5, l15;
    if (laFile >> l1 >> l5 >> l15) stats.loadAvg = {l1, l5, l15};

    stats.cpuUsage = calculateCPU();
    stats.temperatures = readTemperatures();

    std::lock_guard<std::mutex> lock(dataMutex);
    currentStats = stats;
    history.push_back(stats);
    if (history.size() > maxHistory) history.pop_front();
}

double SystemMonitor::calculateCPU() {
    std::ifstream file("/proc/stat");
    std::string cpuLabel;
    unsigned long long u, n, s, i, io, irq, sirq, steal;
    file >> cpuLabel >> u >> n >> s >> i >> io >> irq >> sirq >> steal;

    unsigned long long currentIdle = i + io;
    unsigned long long currentTotal = u + n + s + i + io + irq + sirq + steal;

    unsigned long long dIdle = currentIdle - lastCPU.idle;
    unsigned long long dTotal = currentTotal - lastCPU.total;

    lastCPU.idle = currentIdle;
    lastCPU.total = currentTotal;

    return (dTotal == 0) ? 0.0 : (1.0 - static_cast<double>(dIdle) / dTotal) * 100.0;
}

std::map<std::string, double> SystemMonitor::readTemperatures() {
    std::map<std::string, double> temps;
    for (int i = 0; i < 10; ++i) {
        std::string path = "/sys/class/thermal/thermal_zone" + std::to_string(i) + "/";
        std::ifstream tFile(path + "temp");
        std::ifstream typeFile(path + "type");
        if (tFile && typeFile) {
            double raw;
            std::string type;
            tFile >> raw;
            typeFile >> type;
            temps[type] = raw / 1000.0;
        }
    }
    return temps;
}

void SystemMonitor::updateProcessList() {
    std::vector<ProcessInfo> procs;
    DIR* dir = opendir("/proc");
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            int pid = std::stoi(entry->d_name);
            std::ifstream statFile("/proc/" + std::to_string(pid) + "/stat");
            if (!statFile) continue;

            std::string comm, tmp;
            long rss;
            
            statFile >> tmp >> comm; 
            for(int i=0; i<21; ++i) statFile >> tmp;
            statFile >> rss;

            if (!comm.empty() && comm.front() == '(') comm = comm.substr(1, comm.size() - 2);

            procs.push_back({pid, comm, 0.0, static_cast<double>(rss * sysconf(_SC_PAGESIZE)) / (1024.0 * 1024.0)});
        }
    }
    closedir(dir);

    std::sort(procs.begin(), procs.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
        return a.memUsageMB > b.memUsageMB;
    });

    if (procs.size() > 10) procs.resize(10);

    std::lock_guard<std::mutex> lock(dataMutex);
    topProcesses = procs;
}

SystemStats SystemMonitor::getLatestStats() {
    std::lock_guard<std::mutex> lock(dataMutex);
    return currentStats;
}

std::vector<ProcessInfo> SystemMonitor::getTopProcesses() {
    std::lock_guard<std::mutex> lock(dataMutex);
    return topProcesses;
}

std::vector<SystemStats> SystemMonitor::getHistory() {
    std::lock_guard<std::mutex> lock(dataMutex);
    return {history.begin(), history.end()};
}