// ===== Global Variables =====
let lastAlertState = 0;
let lastData = { healthScore: 100 };
let historicalData = [];
let maxRms = 0, totalAlerts = 0;
let fftMode = 'compare'; // 'm1', 'm2', 'compare'
let websocket;

// Pentru statistici RMS și istoric
let rmsHistory = [];           // buffer pentru ultimele valori RMS (M1)
let maxHistoricalPoints = 50;  // puncte maxime în graficul istoric
window.startTime = Date.now(); // momentul pornirii pentru calcul timp funcționare

// Chart Globals
let fftChart, historicalChart;

// Toggle statistici (Motor 1 / Motor 2 / Compară)
let statsMode = 'm1';
let btnStatsM1, btnStatsM2, btnStatsCompare;

// DOM Elements
let healthProgress, healthText, healthLabel, alertsList;
let sliderM1, sliderM1Value, sliderM2, sliderM2Value;
let btnFftM1, btnFftM2, btnFftCompare;

// Circumference for Health Circle
let circumference;

// ===== STARTUP LOGIC =====
document.addEventListener('DOMContentLoaded', () => {
    // 1. Assign DOM Elements
    healthProgress = document.getElementById('healthProgress');
    healthText     = document.getElementById('healthText');
    healthLabel    = document.getElementById('healthLabel');
    alertsList     = document.getElementById('alertsList');
    
    sliderM1       = document.getElementById("sliderM1");
    sliderM1Value  = document.getElementById("sliderM1Value");
    sliderM2       = document.getElementById("sliderM2");
    sliderM2Value  = document.getElementById("sliderM2Value");
    
    btnFftM1       = document.getElementById('btnFftM1');
    btnFftM2       = document.getElementById('btnFftM2');
    btnFftCompare  = document.getElementById('btnFftCompare');

    // 2. Setup Health Circle
    if (healthProgress) {
        const radius = healthProgress.r.baseVal.value;
        circumference = 2 * Math.PI * radius;
        healthProgress.style.strokeDasharray = `${circumference} ${circumference}`;
    }

    // 3. Initialize Charts
    initCharts();

    // 4. Setup Event Listeners
    if(document.getElementById('btnM1')) document.getElementById('btnM1').onclick = () => sendCmd({ cmd: 'toggleM1' });
    if(document.getElementById('btnM2')) document.getElementById('btnM2').onclick = () => sendCmd({ cmd: 'toggleM2' });
    
    if(document.getElementById('btnEstop')) {
        document.getElementById('btnEstop').onclick = () => {
            sendCmd({ cmd: 'ESTOP' });
            addAlert("OPRIRE DE URGENȚĂ ACTIVATĂ", 'crit');
            showToast('OPRIRE DE URGENȚĂ ACTIVATĂ', 'error');
        };
    }

    // Profile Buttons
    if(document.getElementById('btnProfileNormal'))  document.getElementById('btnProfileNormal').onclick  = () => { sendCmd({ cmd: 'set_profile', val: 0 }); updateProfileDesc(0); };
    if(document.getElementById('btnProfileEconomy')) document.getElementById('btnProfileEconomy').onclick = () => { sendCmd({ cmd: 'set_profile', val: 1 }); updateProfileDesc(1); };
    if(document.getElementById('btnProfileSync'))    document.getElementById('btnProfileSync').onclick    = () => { sendCmd({ cmd: 'set_profile', val: 2 }); updateProfileDesc(2); };

    // Slider Logic
    if (sliderM1) {
        sliderM1.addEventListener("input", () => { if(sliderM1Value) sliderM1Value.textContent = sliderM1.value; });
        sliderM1.addEventListener("change", () => {
            sendCmd({ cmd: "set_m1_speed", val: parseInt(sliderM1.value) });
        });
    }

    if (sliderM2) {
        sliderM2.addEventListener("input", () => { if(sliderM2Value) sliderM2Value.textContent = sliderM2.value; });
        sliderM2.addEventListener("change", () => {
            sendCmd({ cmd: "set_m2_speed", val: parseInt(sliderM2.value) });
        });
    }

    // FFT Controls
    if(btnFftM1) btnFftM1.onclick = () => { fftMode = 'm1'; updateFftVisibility(); };
    if(btnFftM2) btnFftM2.onclick = () => { fftMode = 'm2'; updateFftVisibility(); };
    if(btnFftCompare) btnFftCompare.onclick = () => { fftMode = 'compare'; updateFftVisibility(); };

    // Stats Controls (Motor 1 / Motor 2 / Compară)
    btnStatsM1 = document.getElementById('btnStatsM1');
    btnStatsM2 = document.getElementById('btnStatsM2');
    btnStatsCompare = document.getElementById('btnStatsCompare');

    if (btnStatsM1) btnStatsM1.onclick = () => { statsMode = 'm1'; updateStatsVisibility(); };
    if (btnStatsM2) btnStatsM2.onclick = () => { statsMode = 'm2'; updateStatsVisibility(); };
    if (btnStatsCompare) btnStatsCompare.onclick = () => { statsMode = 'compare'; updateStatsVisibility(); };

    // inițial arătăm Motor 1
    updateStatsVisibility();

    // Navigation & Theme
    setupNavAndTheme();

    function updateStatsVisibility() {
        const c1 = document.getElementById('stats-motor-1');
        const c2 = document.getElementById('stats-motor-2');
        const cc = document.getElementById('stats-compare');

        if (c1) c1.style.display = 'none';
        if (c2) c2.style.display = 'none';
        if (cc) cc.style.display = 'none';

        if (btnStatsM1) btnStatsM1.classList.remove('active');
        if (btnStatsM2) btnStatsM2.classList.remove('active');
        if (btnStatsCompare) btnStatsCompare.classList.remove('active');

        if (statsMode === 'm1') {
            if (c1) c1.style.display = 'block';
            if (btnStatsM1) btnStatsM1.classList.add('active');
        } else if (statsMode === 'm2') {
            if (c2) c2.style.display = 'block';
            if (btnStatsM2) btnStatsM2.classList.add('active');
        } else { // compare
            if (cc) cc.style.display = 'block';
            if (btnStatsCompare) btnStatsCompare.classList.add('active');
        }
        }

    // 5. Start Connection
    initWebSocket();
});

// ===== Helper Functions =====

function sendCmd(json) {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify(json));
    }
}

function updateProfileDesc(profileId) {
    const descEl = document.getElementById('profileDescriptionText');
    const btns = document.querySelectorAll('.btn-profile');
    btns.forEach(b => b.classList.remove('active'));

    let text = "";
    if (profileId === 0) {
        text = "<strong>Normal:</strong> Motoarele funcționează la viteza setată manual, fără restricții.";
        document.getElementById('btnProfileNormal')?.classList.add('active');
    } else if (profileId === 1) {
        text = "<strong>Economic:</strong> Viteza este limitată la 60% pentru a economisi energie și a reduce uzura.";
        document.getElementById('btnProfileEconomy')?.classList.add('active');
    } else if (profileId === 2) {
        text = "<strong>Sincron:</strong> Motorul 2 pornește automat la 3 secunde după Motorul 1 pentru a reduce vârful de curent.";
        document.getElementById('btnProfileSync')?.classList.add('active');
    }
    
    if(descEl) descEl.innerHTML = text;
}
// Health per motor (cercurile noi)
function setHealthMotor(motorId, health) {
  const circleId = `health-circle-m${motorId}`;
  const percentId = `health-percent-m${motorId}`;
  const statusId = `health-status-m${motorId}`;

  const circle = document.getElementById(circleId);
  const percent = document.getElementById(percentId);
  const status = document.getElementById(statusId);

  if (!circle || !percent || !status) return;

  const clamped = Math.max(0, Math.min(100, health));
  circle.style.strokeDasharray = `${clamped}, 100`;
  percent.textContent = `${Math.round(clamped)}%`;

  const baseColor = motorId === 1 ? 'var(--accent-color)' : '#FFA500';
  
  if (clamped >= 80) {
    circle.style.stroke = baseColor;
    status.textContent = 'Sănătos';
    status.style.color = baseColor;
  } else if (clamped >= 50) {
    circle.style.stroke = '#FFC107';
    status.textContent = 'Atenție';
    status.style.color = '#FFC107';
  } else {
    circle.style.stroke = '#F44336';
    status.textContent = 'Critic';
    status.style.color = '#F44336';
  }
}

// ===== WebSocket Logic =====
function initWebSocket() {
  const isLocalAP = window.location.hostname === "" || window.location.hostname === "192.168.4.1";
  const wsHost = isLocalAP ? "192.168.4.1" : window.location.hostname;
  const url = `ws://${wsHost}/ws`;

  const statusElem = document.getElementById('connectionStatus');
  const lastUpdateElem = document.getElementById('lastUpdate');

  websocket = new WebSocket(url);

  websocket.onopen = () => {
    if(statusElem) statusElem.innerHTML = '<span style="color: var(--green);">●</span> Conectat';
  };

  websocket.onmessage = (event) => {
    const d = JSON.parse(event.data);
    if (d.type !== 'telemetry') return;

    const rms1 = d.rms1 ?? 0;
    const rms2 = d.rms2 ?? 0;
    const m1_spd = d.m1_speed ?? 0;
    const m2_spd = d.m2_speed ?? 0;

    // --- 1. HEALTH PER MOTOR ---
    setHealthMotor(1, d.health1 ?? 100);
    setHealthMotor(2, d.health2 ?? 100);

    // --- 2. MOTOR CONTROL CARDS ---
    updateStatusBadge('m1_status', d.motor1State);
    updateStatusBadge('m2_status', d.motor2State);
    
    if (document.getElementById('m1_speed')) document.getElementById('m1_speed').textContent = m1_spd;
    if (document.getElementById('m2_speed')) document.getElementById('m2_speed').textContent = m2_spd;
    if (document.getElementById('m1_rms')) document.getElementById('m1_rms').textContent = rms1.toFixed(2);
    if (document.getElementById('m2_rms')) document.getElementById('m2_rms').textContent = rms2.toFixed(2);

    const sliderM1 = document.getElementById('sliderM1');
    const sliderM2 = document.getElementById('sliderM2');
    const sliderM1Value = document.getElementById('sliderM1Value');
    const sliderM2Value = document.getElementById('sliderM2Value');
    
    if (sliderM1 && m1_spd !== undefined) sliderM1.value = m1_spd;
    if (sliderM2 && m2_spd !== undefined) sliderM2.value = m2_spd;
    if (sliderM1Value) sliderM1Value.textContent = m1_spd;
    if (sliderM2Value) sliderM2Value.textContent = m2_spd;

    // --- 3. STATISTICS MOTOR 1 ---
    if (document.getElementById('stat_rms_current')) document.getElementById('stat_rms_current').textContent = rms1.toFixed(2);
    if (document.getElementById('stat-peak-m1')) document.getElementById('stat-peak-m1').textContent = (d.peak1 ?? 0).toFixed(2);
    if (document.getElementById('stat-crest-m1')) document.getElementById('stat-crest-m1').textContent = (d.crest1 ?? 0).toFixed(2);
    if (document.getElementById('stat-freq-m1')) document.getElementById('stat-freq-m1').textContent = (d.dom_f1 ?? 0).toFixed(1) + ' Hz';

    // --- 4. STATISTICS MOTOR 2 ---
    if (document.getElementById('stat-rms-m2')) document.getElementById('stat-rms-m2').textContent = rms2.toFixed(2);
    if (document.getElementById('stat-peak-m2')) document.getElementById('stat-peak-m2').textContent = (d.peak2 ?? 0).toFixed(2);
    if (document.getElementById('stat-crest-m2')) document.getElementById('stat-crest-m2').textContent = (d.crest2 ?? 0).toFixed(2);
    if (document.getElementById('stat-freq-m2')) document.getElementById('stat-freq-m2').textContent = (d.dom_f2 ?? 0).toFixed(1) + ' Hz';

    // --- 5. COMPARISON TABLE ---
    if (document.getElementById('comp-rms-m1')) document.getElementById('comp-rms-m1').textContent = rms1.toFixed(2);
    if (document.getElementById('comp-rms-m2')) document.getElementById('comp-rms-m2').textContent = rms2.toFixed(2);
    if (document.getElementById('comp-peak-m1')) document.getElementById('comp-peak-m1').textContent = (d.peak1 ?? 0).toFixed(2);
    if (document.getElementById('comp-peak-m2')) document.getElementById('comp-peak-m2').textContent = (d.peak2 ?? 0).toFixed(2);
    if (document.getElementById('comp-crest-m1')) document.getElementById('comp-crest-m1').textContent = (d.crest1 ?? 0).toFixed(2);
    if (document.getElementById('comp-crest-m2')) document.getElementById('comp-crest-m2').textContent = (d.crest2 ?? 0).toFixed(2);
    if (document.getElementById('comp-freq-m1')) document.getElementById('comp-freq-m1').textContent = (d.dom_f1 ?? 0).toFixed(1) + ' Hz';
    if (document.getElementById('comp-freq-m2')) document.getElementById('comp-freq-m2').textContent = (d.dom_f2 ?? 0).toFixed(1) + ' Hz';

    // --- 6. RMS ROLLING STATS (pentru stats-grid vechi) ---
    rmsHistory.push(rms1);
    if (rmsHistory.length > 10) rmsHistory.shift();
    
    const avg = rmsHistory.reduce((a, b) => a + b, 0) / rmsHistory.length;
    const variance = rmsHistory.reduce((sum, val) => sum + Math.pow(val - avg, 2), 0) / rmsHistory.length;
    const stddev = Math.sqrt(variance);
    
    if (document.getElementById('stat_rms_avg')) document.getElementById('stat_rms_avg').textContent = avg.toFixed(2);
    if (document.getElementById('stat_rms_stddev')) document.getElementById('stat_rms_stddev').textContent = stddev.toFixed(2);
    if (document.getElementById('stat_crest_factor')) document.getElementById('stat_crest_factor').textContent = (d.crest1 ?? 0).toFixed(2);

    // Trend
    if (rmsHistory.length >= 5) {
        const recent = rmsHistory.slice(-5);
        const older = rmsHistory.slice(0, 5);
        const recentAvg = recent.reduce((a, b) => a + b, 0) / recent.length;
        const olderAvg = older.reduce((a, b) => a + b, 0) / older.length;
        const trendElem = document.getElementById('stat_trend');
        if (trendElem) {
        if (recentAvg > olderAvg + 0.5) trendElem.textContent = '📈 Creștere';
        else if (recentAvg < olderAvg - 0.5) trendElem.textContent = '📉 Scădere';
        else trendElem.textContent = '➡️ Stabil';
        }
    }

    // --- 7. PROFILE ---
    if (d.profile !== undefined) updateProfileDesc(d.profile);

    // --- 8. LAST UPDATE TIMESTAMP ---
    const lastUpdateElem = document.getElementById('lastUpdate');
    if (lastUpdateElem) lastUpdateElem.textContent = new Date().toLocaleTimeString();

    // --- 9. FFT CHARTS ---
    if (fftChart) {
        if (d.fft1 && d.fft1.length > 0) fftChart.data.datasets[0].data = d.fft1;
        if (d.fft2 && d.fft2.length > 0) fftChart.data.datasets[1].data = d.fft2;
        fftChart.update('none');
    }

    // --- 10. HISTORICAL DATA (RMS TREND) ---
    const timestamp = new Date().toLocaleTimeString();
    historicalData.push({ x: timestamp, y1: rms1, y2: rms2 });
    if (historicalData.length > maxHistoricalPoints) historicalData.shift();

    if (rms1 > maxRms) maxRms = rms1;
    if (rms2 > maxRms) maxRms = rms2;
    const maxRmsElem = document.getElementById('analysis-max-rms');
    if (maxRmsElem) maxRmsElem.textContent = maxRms.toFixed(2);

    if (historicalChart) {
        historicalChart.data.labels = historicalData.map(p => p.x);
        historicalChart.data.datasets[0].data = historicalData.map(p => p.y1);
        historicalChart.data.datasets[1].data = historicalData.map(p => p.y2);
        historicalChart.update('none');
    }

    // --- 11. FOOTER STATS ---
    const totalAlertsElem = document.getElementById('analysis-total-alerts');
    if (totalAlertsElem) totalAlertsElem.textContent = totalAlerts;

    const totalRuntimeElem = document.getElementById('analysis-total-runtime');
    if (totalRuntimeElem) {
        const runtime = Math.floor((Date.now() - (window.startTime || Date.now())) / 1000);
        const hours = Math.floor(runtime / 3600);
        const minutes = Math.floor((runtime % 3600) / 60);
        const seconds = runtime % 60;
        totalRuntimeElem.textContent = `${hours}h ${minutes}m ${seconds}s`;
    }

    // --- 12. MOTORS PAGE UPDATE ---
    updateMotorsPage(d);

    // --- 13. ALERTS ---
    updateAlerts(d);
    };


  websocket.onclose = () => {
    if(statusElem) statusElem.innerHTML = '<span style="color: var(--red);">●</span> Deconectat. Reconectare...';
    setTimeout(initWebSocket, 2000);
  };
}

function updateStatusBadge(id, isOn) {
    const elem = document.getElementById(id);
    if (!elem) return;
    if (isOn) { elem.textContent = "PORNIT"; elem.classList.remove("off"); elem.classList.add("on"); } 
    else { elem.textContent = "OPRIT"; elem.classList.remove("on"); elem.classList.add("off"); }
}

function updateMotorsPage(d) {
    ['1','2'].forEach(i => {
      const isOn = (i === '1') ? d.motor1State : d.motor2State;
      const stEl = document.getElementById(`motor-detail-status-${i}`);
      if(stEl) stEl.textContent = isOn ? 'PORNIT' : 'OPRIT';
      
      const spEl = document.getElementById(`motor-detail-speed-${i}`);
      if(spEl) spEl.textContent = d[`m${i}_speed`] ?? 0;
    });
}

function updateAlerts(d) {
  document.body.style.setProperty('--alert-bg', null);
  if (d.alertState === 1) {
    if (lastAlertState !== 1) { addAlert(`Avertisment Vibrații`, 'warn'); showToast(`Vibrații Ridicate`, 'warning'); }
    document.body.style.setProperty('--alert-bg', '#2d2d1b');
  } else if (d.alertState === 2) {
    if (lastAlertState !== 2) { addAlert(`ALARMĂ CRITICĂ`, 'crit'); showToast(`ALARMĂ CRITICĂ`, 'error'); }
    document.body.style.setProperty('--alert-bg', '#3b1f1f');
  } else {
    if (lastAlertState !== 0 && lastAlertState !== undefined) addAlert('Sistem Normal.', 'info');
  }
}

function setupNavAndTheme() {
    const navIcons = document.querySelectorAll('.nav-icon');
    navIcons.forEach(icon => {
        icon.addEventListener('click', () => {
        document.querySelector('.nav-icon.active')?.classList.remove('active');
        icon.classList.add('active');
        const targetId = icon.getAttribute('data-target');
        document.querySelector('.page-content.active-page')?.classList.remove('active-page');
        document.getElementById(targetId).classList.add('active-page');
        });
    });

    const themeToggle = document.getElementById('themeToggle');
    if(themeToggle) {
        const savedTheme = localStorage.getItem('theme') || 'dark';
        applyTheme(savedTheme);
        themeToggle.checked = savedTheme === 'light';
        themeToggle.addEventListener('change', () => {
            const newTheme = document.body.classList.contains('light-theme') ? 'dark' : 'light';
            applyTheme(newTheme);
            localStorage.setItem('theme', newTheme);
        });
    } else { applyTheme('dark'); }
}

function applyTheme(theme) {
    document.body.classList.toggle('light-theme', theme === 'light');
    if(fftChart) { fftChart.options = getChartOptions(); fftChart.update(); }
    if(historicalChart) { historicalChart.options = getChartOptions(); historicalChart.update(); }
}

function getChartOptions() {
    return {
        scales: {
            y: { beginAtZero: true, ticks: { color: getComputedStyle(document.body).getPropertyValue('--text-color') } },
            x: { ticks: { color: getComputedStyle(document.body).getPropertyValue('--text-color'), autoSkip: true, maxTicksLimit: 20 } }
        },
        animation: false,
        plugins: { legend: { labels: { color: getComputedStyle(document.body).getPropertyValue('--text-color') } } }
    };
}

function initCharts() {
    const fftCtx = document.getElementById('fftChart');
    if (fftCtx) {
        fftChart = new Chart(fftCtx.getContext('2d'), {
            type: 'bar',
            data: { 
                labels: [], 
                datasets: [
                    { label: 'Motor 1', data: [], backgroundColor: 'rgba(159, 122, 234, 0.6)', borderColor: 'rgba(159, 122, 234, 1)', borderWidth: 1 },
                    { label: 'Motor 2', data: [], backgroundColor: 'rgba(255, 159, 64, 0.6)', borderColor: 'rgba(255, 159, 64, 1)', borderWidth: 1 }
                ] 
            },
            options: getChartOptions()
        });
        const samplingFrequency = 400, samples = 256;
        for (let i = 0; i < samples / 2; i++) {
            fftChart.data.labels.push((i * samplingFrequency / samples).toFixed(1) + ' Hz');
        }
        updateFftVisibility();
    }

    const histCtx = document.getElementById('historicalChart');
    if (histCtx) {
        historicalChart = new Chart(histCtx.getContext('2d'), {
            type: 'line',
            data: { 
                labels: [], 
                datasets: [
                    { label: 'RMS (M1)', data: [], borderColor: 'var(--accent-color)', backgroundColor: 'rgba(76, 175, 80, 0.1)', fill: false, tension: 0.4 },
                    { label: 'RMS (M2)', data: [], borderColor: '#FFA500', backgroundColor: 'rgba(255, 165, 0, 0.1)', fill: false, tension: 0.4 }
                ] 
            },
            options: getChartOptions()
        });
    }
}

function updateFftVisibility() {
  if (!fftChart) return;

  if(btnFftM1) btnFftM1.classList.remove('active');
  if(btnFftM2) btnFftM2.classList.remove('active');
  if(btnFftCompare) btnFftCompare.classList.remove('active');

  if (fftMode === 'm1') {
    fftChart.data.datasets[0].hidden = false;
    fftChart.data.datasets[1].hidden = true;
    if(btnFftM1) btnFftM1.classList.add('active');
    fftChart.options.plugins.legend.display = false;    // doar M1 → fără legendă
  } else if (fftMode === 'm2') {
    fftChart.data.datasets[0].hidden = true;
    fftChart.data.datasets[1].hidden = false;
    if(btnFftM2) btnFftM2.classList.add('active');
    fftChart.options.plugins.legend.display = false;    // doar M2 → fără legendă
  } else {
    fftChart.data.datasets[0].hidden = false;
    fftChart.data.datasets[1].hidden = false;
    if(btnFftCompare) btnFftCompare.classList.add('active');
    fftChart.options.plugins.legend.display = true;     // Compară → legendă ON
  }

  fftChart.update();
}
