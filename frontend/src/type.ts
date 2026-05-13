export interface ProcessInfo {
    pid: number;
    name: string;
    cpuUsage: number;
    memUsageMB: number;
}

export interface MonitorData {
    cpu: number;
    ram_usage_perc: number;
    ram_used_gb: number;
    load_avg: number[];
    temps?: Record<string, number>;
    procs?: ProcessInfo[];
}