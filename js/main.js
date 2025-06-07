// --- JS: GESTIONNAIRE COMPLET ET FONCTIONNEL ---
const themes = ['dark', 'light', 'cyberpunk', 'foundry', 'solarpunk', 'vhs'];
const themeIcons = { dark: '🌙', light: '☀️', cyberpunk: '🕶️', foundry: '🔨', solarpunk: '🌱', vhs: '📼' };
const USE_SIMULATION = true;
const ESP32_IP_ADDRESS = '192.168.1.100'; // Note: This will be client-side only without a backend
const UPDATE_INTERVAL = 2000;

// --- DOM Elements ---
let productionChart;
const themeToggle = document.getElementById('theme-toggle');
const settingsForm = document.getElementById('settings-form');
const powerValueEl = document.getElementById('power-value');
const voltageValueEl = document.getElementById('voltage-value');
const currentValueEl = document.getElementById('current-value');
const ldrHValueEl = document.getElementById('ldr-h-value');
const ldrBValueEl = document.getElementById('ldr-b-value');
const ldrGValueEl = document.getElementById('ldr-g-value');
const ldrDValueEl = document.getElementById('ldr-d-value');
const ldrHBarEl = document.getElementById('ldr-h-bar');
const ldrBBarEl = document.getElementById('ldr-b-bar');
const ldrGBarEl = document.getElementById('ldr-g-bar');
const ldrDBarEl = document.getElementById('ldr-d-bar');
const logBox = document.getElementById('log-box');
const chartCanvas = document.getElementById('productionChart'); // Corrected from user HTML which might have a typo if canvas ID is different

// --- Fonctions ---
function setTheme(theme) { // Assuming "setTeam" was a typo for "setTheme"
    document.body.className = '';
    document.body.classList.add(`${theme}-mode`);
    if (themeToggle) { // Check if element exists
        themeToggle.textContent = themeIcons[theme];
    }
    localStorage.setItem('solar_theme', theme);
    if (theme === 'solarpunk') {
        createBubbles();
    }
    if (productionChart) {
        updateChartColors();
    }
}

function createBubbles() {
    const wrapper = document.getElementById('bubble-wrapper');
    if (!wrapper || wrapper.childElementCount > 0) return; // Check if wrapper exists
    for (let i = 0; i < 7; i++) {
        const bubble = document.createElement('div');
        bubble.className = 'bubble';
        wrapper.appendChild(bubble);
    }
}

function updateUI(data) {
    if (powerValueEl) powerValueEl.textContent = `${data.power.toFixed(2)} W`;
    if (voltageValueEl) voltageValueEl.textContent = `${data.voltage.toFixed(2)} V`;
    if (currentValueEl) currentValueEl.textContent = `${data.current.toFixed(2)} A`;

    if (ldrGValueEl) ldrGValueEl.textContent = data.ldr_g;
    if (ldrDValueEl) ldrDValueEl.textContent = data.ldr_d;
    if (ldrHValueEl) ldrHValueEl.textContent = data.ldr_h;
    if (ldrBValueEl) ldrBValueEl.textContent = data.ldr_b;

    const maxLdr = 1023;
    if (ldrGBarEl) ldrGBarEl.style.width = `${(data.ldr_g / maxLdr) * 100}%`;
    if (ldrDBarEl) ldrDBarEl.style.width = `${(data.ldr_d / maxLdr) * 100}%`;
    if (ldrHBarEl) ldrHBarEl.style.width = `${(data.ldr_h / maxLdr) * 100}%`;
    if (ldrBBarEl) ldrBBarEl.style.width = `${(data.ldr_b / maxLdr) * 100}%`;

    document.querySelectorAll('.value, .ldr-list span:last-of-type').forEach(el => {
        el.classList.remove('data-flash');
        void el.offsetWidth; // Trigger reflow to restart animation
        el.classList.add('data-flash');
    });
}

function simulateData() {
    const d = new Date();
    const timeFactor = (d.getHours() * 3600 + d.getMinutes() * 60 + d.getSeconds()) / 86400;
    const sunCycle = Math.sin(timeFactor * Math.PI);
    let power = 0; let ldr_max = 0;
    if (sunCycle > 0.1) {
        power = Math.max(0, (sunCycle * 11) + (Math.random() - 0.5));
        ldr_max = 300 + sunCycle * 700;
    }
    return {
        power: Math.min(power, 11),
        voltage: power > 0 ? 6.5 + (power / 11) * 0.5 : 0,
        current: power > 0 ? power / (6.5 + (power / 11) * 0.5) : 0,
        ldr_g: Math.round(ldr_max * (0.5 + sunCycle * 0.4) + Math.random()*20),
        ldr_d: Math.round(ldr_max * (0.5 - sunCycle * 0.4) + Math.random()*20),
        ldr_h: Math.round(ldr_max * 0.9 + Math.random()*20),
        ldr_b: Math.round(ldr_max * 0.2 + Math.random()*20),
    };
}

function logEvent(message, type = 'info') {
    if (!logBox) return; // Check if logBox exists
    const time = new Date().toLocaleTimeString();
    const newLog = document.createElement('div');
    let color;
    switch(type) {
        case 'error': color = '#f43f5e'; break;
        case 'success': color = '#34d399'; break;
        default: color = 'var(--text-muted)'; // Ensure this CSS variable is available or provide fallback
    }
    newLog.innerHTML = `<span style="color:${color};">[${time}] ${message}</span>`;
    logBox.prepend(newLog); // Add to top
}

function initChart() {
    if (!chartCanvas) {
        console.error("Chart canvas element (productionChart) not found!");
        return;
    }
    if (typeof Chart === 'undefined') {
        console.error("Chart.js is not loaded!");
        return;
    }
    productionChart = new Chart(chartCanvas, {
        type: 'line',
        data: { datasets: [{ // Initialize with an empty dataset structure if needed
            label: 'Production (Watts)', // Example label
            data: [],
            borderColor: 'var(--accent-color)', // Example, will be updated by updateChartColors
            backgroundColor: 'rgba(0, 255, 204, 0.1)', // Example
            tension: 0.3
        }]
      },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                x: {
                    type: 'time',
                    time: { unit: 'hour', displayFormats: { hour: 'HH:mm' } },
                    min: new Date().setHours(0,0,0,0),
                    max: new Date().setHours(23,59,59,999),
                    ticks: { color: 'var(--text-muted)' }, // Initial color
                    grid: { color: 'var(--border-color)' }  // Initial color
                },
                y: {
                    beginAtZero: true,
                    max: 12, // As per user's code
                    ticks: { color: 'var(--text-muted)' }, // Initial color
                    grid: { color: 'var(--border-color)' }  // Initial color
                }
            },
            elements: {
                line: { tension: 0.3 },
                point: { radius: 0 }
            },
            plugins: {
                legend: {
                    labels: {
                        color: 'var(--text-muted)' // Initial color
                    }
                }
            }
        }
    });
    // Add a dummy data point to initialize the hourly x-axis correctly if no data is immediately available
    // This is a common workaround for time-series charts to show the initial scale
    const now = new Date();
    const startOfHour = new Date(now.getFullYear(), now.getMonth(), now.getDate(), now.getHours(), 0, 0, 0);
    if (productionChart.data.datasets[0].data.length === 0) {
        productionChart.data.datasets[0].data.push({x: startOfHour.getTime(), y: null}); // Add a null point
        productionChart.update('none'); // Update without animation
    }
}

function updateChartColors() {
    if (!productionChart) return;
    const bodyStyles = getComputedStyle(document.body);
    const newTextColor = bodyStyles.getPropertyValue('--text-muted').trim();
    const newGridColor = bodyStyles.getPropertyValue('--border-color').trim();
    const newAccentColor = bodyStyles.getPropertyValue('--accent-color').trim();

    productionChart.options.scales.x.ticks.color = newTextColor;
    productionChart.options.scales.y.ticks.color = newTextColor;
    productionChart.options.scales.x.grid.color = newGridColor;
    productionChart.options.scales.y.grid.color = newGridColor;
    productionChart.options.plugins.legend.labels.color = newTextColor;

    if (productionChart.data.datasets[0]) {
        productionChart.data.datasets[0].borderColor = newAccentColor;
        productionChart.data.datasets[0].backgroundColor = newAccentColor.replace(')', ', 0.1)').replace('rgb', 'rgba'); // Basic conversion to rgba
    }
    productionChart.update('none'); // 'none' prevents animation during color updates
}

// --- Initialisation ---
document.addEventListener('DOMContentLoaded', () => {
    // Ensure DOM elements are truly available
    // Re-assigning here in case they were not ready globally
    const themeToggleLocal = document.getElementById('theme-toggle');
    const settingsFormLocal = document.getElementById('settings-form');
    // ... (re-assign other global DOM element vars if strict scoping is an issue, though typically not for IDs)

    initChart(); // Initialize chart
    const savedTheme = localStorage.getItem('solar_theme') || 'vhs'; // Default to vhs as per user code
    setTheme(savedTheme); // Set theme (assuming setTeam was typo for setTheme)

    logEvent("Data Hub // Retro Glitch Edition // Online");

    mainLoop(); // Initial data load/simulation
    setInterval(mainLoop, UPDATE_INTERVAL); // Set up interval

    if (themeToggleLocal) {
        themeToggleLocal.addEventListener('click', () => {
            const currentTheme = localStorage.getItem('solar_theme') || 'vhs';
            const currentIndex = themes.indexOf(currentTheme);
            const nextIndex = (currentIndex + 1) % themes.length;
            setTheme(themes[nextIndex]); // setTheme
        });
    } else {
        console.error("Theme toggle button not found during event listener setup.");
    }

    if (settingsFormLocal) {
        settingsFormLocal.addEventListener('submit', (e) => {
            e.preventDefault();
            // Example of getting values:
            const zoneMorte = document.getElementById('zoneMorte').value;
            logEvent(`Paramètres sauvegardés (simulation). Zone Morte: ${zoneMorte}`, "success");
            // Actual settings application would go here (e.g., API call)
        });
    } else {
        console.error("Settings form not found during event listener setup.");
    }
});


async function mainLoop() { // Marked as async if future fetch calls are made
    let data;
    if (USE_SIMULATION) {
        data = simulateData();
        // Simulate pushing data to chart
        if (productionChart && data) {
            const now = new Date();
            // Keep only last 24 hours of data (approx)
            const twentyFourHoursAgo = now.getTime() - (24 * 60 * 60 * 1000);

            productionChart.data.datasets[0].data.push({ x: now.getTime(), y: data.power });

            // Filter out old data points
            productionChart.data.datasets[0].data = productionChart.data.datasets[0].data.filter(p => p.x >= twentyFourHoursAgo);

            // Update chart x-axis min/max to scroll
            productionChart.options.scales.x.min = twentyFourHoursAgo;
            productionChart.options.scales.x.max = now.getTime();

            productionChart.update();
        }
    } else {
        // Placeholder for actual data fetching e.g. from ESP32
        // try {
        //     const response = await fetch(`http://${ESP32_IP_ADDRESS}/data`);
        //     if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
        //     data = await response.json();
        //     logEvent("Data received from ESP32", "success");
        // } catch (error) {
        //     logEvent(`Error fetching data: ${error.message}`, "error");
        //     data = simulateData(); // Fallback to simulation on error
        //     logEvent("Falling back to simulated data due to fetch error.", "info");
        // }
    }

    if (data) {
        updateUI(data);
    }
}

// Minor patch for chartCanvas ID if it was different in original user HTML
// This script assumes chartCanvas = document.getElementById('productionChart');
// If the ID in user's HTML was different, this global var needs to match it.
// The HTML provided to the subtask for index.html uses 'productionChart', so it should be fine.
