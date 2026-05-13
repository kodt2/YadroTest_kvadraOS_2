import { MonitorData, ProcessInfo } from './type.js';

declare const Chart: any;

const cpuHistory: number[] = [];
const labels: string[] = [];
const MAX_POINTS = 60;

const ctx = (document.getElementById('cpuChart') as HTMLCanvasElement).getContext('2d');
const chart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: labels,
        datasets: [{
            label: 'CPU Load %',
            data: cpuHistory,
            borderColor: '#58a6ff',
            backgroundColor: 'rgba(88, 166, 255, 0.1)',
            borderWidth: 2,
            fill: true,
            pointRadius: 0,
            tension: 0.4
        }]
    },
    options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: false,
        scales: {
            y: { min: 0, max: 100, grid: { color: '#30363d' } },
            x: { display: false }
        },
        plugins: { legend: { display: false } }
    }
});

const ws = new WebSocket(`ws://${window.location.hostname}:8081`);

ws.onmessage = (event) => {
    const data: MonitorData = JSON.parse(event.data);
    updateUI(data);
};

function updateUI(data: any) {
    if (data.cpu !== undefined) {
        document.getElementById('cpu-perc')!.innerText = `${data.cpu.toFixed(1)}%`;
        document.getElementById('cpu-bar')!.style.width = `${data.cpu}%`;
        updateChart(data.cpu);
    }

    const ram = data.ram_usage_perc; 
    if (ram !== undefined) {
        document.getElementById('ram-perc')!.innerText = `${ram.toFixed(1)}%`;
        document.getElementById('ram-bar')!.style.width = `${ram}%`;
    }

    if (data.temps) {
        updateTemperatures(data.temps);
    }
    
    if (data.procs) {
        updateProcessTable(data.procs);
    }
}

function updateChart(cpu: number) {
    cpuHistory.push(cpu);
    labels.push('');
    if (cpuHistory.length > MAX_POINTS) {
        cpuHistory.shift();
        labels.shift();
    }
    chart.update();
}

function updateTemperatures(temps: Record<string, number>) {
    const container = document.getElementById('temp-grid')!;
    container.innerHTML = '';
    for (const [name, val] of Object.entries(temps)) {
        const div = document.createElement('div');
        div.className = 'temp-card';
        div.innerHTML = `
            <div class="temp-label">${name}</div>
            <div class="temp-value">${val.toFixed(1)}°C</div>
        `;
        container.appendChild(div);
    }
}

function updateProcessTable(procs: ProcessInfo[]) {
    const tbody = document.getElementById('proc-body')!;
    tbody.innerHTML = procs.map(p => `
        <tr>
            <td>${p.pid}</td>
            <td>${p.name.replace(/[()]/g, '')}</td>
            <td>${p.memUsageMB.toFixed(1)} MB</td>
        </tr>
    `).join('');
}

ws.onerror = () => {
    console.error('WebSocket error');
    document.title = 'OFFLINE - Monitor';
};