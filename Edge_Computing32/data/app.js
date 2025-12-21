// ===== Global Variables =====
let lastAlertState = 0;
let lastData = { healthScore: 100 };
let historicalData = [];
let maxRms = 0, totalAlerts = 0;
let fftMode = 'compare'; // 'm1', 'm2', 'compare'
let websocket;

// Variabile pentru sincronizare UI (Debounce/Cooldown)
let lastCmdTimeM1 = 0; 
let lastCmdTimeM2 = 0; 
const CMD_TIMEOUT = 1500; // Ignorăm datele de la server 1.5s după o comandă manuală

// Statistici RMS și istoric
let rmsHistory = [];   
let rmsHistory2 = [];  
let maxHistoricalPoints = 50;
window.startTime = Date.now(); 

// Chart Globals
let fftChart, historicalChart;

// Toggle statistici
let statsMode = 'm1';
let btnStatsM1, btnStatsM2, btnStatsCompare;

// DOM Elements
let healthProgress, healthText, healthLabel, alertsList;
let sliderM1, sliderM1Value, sliderM2, sliderM2Value;
let btnFftM1, btnFftM2, btnFftCompare;

// Circumference for Health Circle
let circumference;

// ===== INTERNATIONALIZATION (i18n) =====
let currentLang = 'ro';
let currentProfileId = -1; 

const translations = {
  ro: {
    header_title: "Panou de control",
    footer_conn: "Conectare...",
    footer_updated: "Actualizat:",
    dash_title: "Sistem de diagnoză", 
    health_title: "Stare tehnică",
    motor1: "Motor 1",
    motor2: "Motor 2",
    health_stable: "În parametri",
    health_warning: "Uzură detectată",
    health_critical: "Defecțiune",
    control_title: "Control motoare",
    status_on: "Pornit",
    status_off: "Oprit",
    speed_txt: "Viteza",
    btn_m1_toggle: "Start/stop motor 1",
    btn_m2_toggle: "Start/stop motor 2",
    speed_m1: "Viteză motor 1:",
    speed_m2: "Viteză motor 2:",
    profile_title: "Profil operare",
    prof_normal: "Normal",
    prof_eco: "Economic",
    prof_sync: "Sincron",
    prof_select: "Selectați un profil.",
    prof_desc_normal: "<strong>Normal:</strong> Motoarele funcționează la viteza setată manual, fără restricții.",
    prof_desc_eco: "<strong>Economic:</strong> Viteza este limitată la 60% pentru a economisi energie.",
    prof_desc_sync: "<strong>Sincron:</strong> Motorul 2 pornește automat la 3 secunde după motorul 1.",
    vib_analysis_title: "Analiză vibrații",
    key_stats: "Statistici cheie (RMS)",
    compare: "Compară",
    metric_rms: "Valoare curentă",
    desc_rms: "RMS (Root Mean Square) indică energia totală a vibrației.",
    metric_peak: "Peak (vârf)",
    desc_peak: "Valoarea maximă înregistrată.",
    metric_crest: "Factor creastă",
    desc_crest: "Raportul dintre Vârf (Peak) și RMS.",
    metric_freq: "Frecvență dominantă",
    desc_freq: "Frecvența cu amplitudine maximă.",
    metric_trend: "Trend general:",
    trend_up: "📈 Creștere",
    trend_down: "📉 Scădere",
    trend_flat: "➡️ Stabil",
    table_metric: "Metrică",
    alerts_history: "Istoric alerte",
    sys_start: "Sistem pornit...",
    btn_estop: "Oprire de urgență",
    alert_warn_msg: "Avertisment vibrații",
    alert_crit_msg: "Alarmă critică",
    alert_norm_msg: "Sistem normal.",
    fft_title: "Spectrogramă vibrații (FFT)",
    compare_full: "Compară (ambele)",
    page_analysis_title: "Analiză istorică",
    card_max_rms: "Max RMS",
    card_total_alerts: "Total alerte",
    card_runtime: "Timp funcționare",
    card_history_title: "Istoric medie RMS",
    page_motors_title: "Detalii motoare",
    card_det_m1: "Detalii motor 1",
    card_det_m2: "Detalii motor 2",
    det_status: "Stare:",
    det_speed: "Viteză curentă:",
    det_runtime: "Timp funcționare:",
    det_cycles: "Cicluri pornire:",
    page_settings_title: "Setări sistem",
    card_esp_title: "Setări ESP32",
    card_esp_desc: "Configurare parametri rețea și limite.",
    btn_esp_access: "Acces setări dispozitiv",
    card_ui_title: "Setări interfață",
    lbl_theme: "Temă vizuală"
  },
  en: {
    header_title: "Control panel",
    footer_conn: "Connecting...",
    footer_updated: "Updated:",
    dash_title: "Diagnostic system",
    health_title: "Technical condition",
    motor1: "Motor 1",
    motor2: "Motor 2",
    health_stable: "Optimal",
    health_warning: "Warning",
    health_critical: "Critical failure",
    control_title: "Motor control",
    status_on: "On",
    status_off: "Off",
    speed_txt: "Speed",
    btn_m1_toggle: "Start/stop motor 1",
    btn_m2_toggle: "Start/stop motor 2",
    speed_m1: "Motor 1 speed:",
    speed_m2: "Motor 2 speed:",
    profile_title: "Operation profile",
    prof_normal: "Normal",
    prof_eco: "Eco",
    prof_sync: "Sync",
    prof_select: "Select a profile.",
    prof_desc_normal: "<strong>Normal:</strong> Motors run at manually set speed without restrictions.",
    prof_desc_eco: "<strong>Eco:</strong> Speed is limited to 60% to save energy.",
    prof_desc_sync: "<strong>Sync:</strong> Motor 2 starts automatically 3 seconds after motor 1.",
    vib_analysis_title: "Vibration analysis",
    key_stats: "Key statistics (RMS)",
    compare: "Compare",
    metric_rms: "Current value",
    desc_rms: "RMS (Root Mean Square) indicates total vibration energy.",
    metric_peak: "Peak",
    desc_peak: "Maximum recorded value.",
    metric_crest: "Crest factor",
    desc_crest: "Ratio between Peak and RMS.",
    metric_freq: "Dominant freq",
    desc_freq: "Frequency with maximum amplitude.",
    metric_trend: "General trend:",
    trend_up: "📈 Rising",
    trend_down: "📉 Falling",
    trend_flat: "➡️ Stable",
    table_metric: "Metric",
    alerts_history: "Alert history",
    sys_start: "System started...",
    btn_estop: "Emergency stop",
    alert_warn_msg: "Vibration warning",
    alert_crit_msg: "Critical alarm",
    alert_norm_msg: "System normal.",
    fft_title: "Vibration spectrogram (FFT)",
    compare_full: "Compare (both)",
    page_analysis_title: "Historical analysis",
    card_max_rms: "Max RMS",
    card_total_alerts: "Total alerts",
    card_runtime: "Runtime",
    card_history_title: "RMS history avg",
    page_motors_title: "Motor details",
    card_det_m1: "Motor 1 details",
    card_det_m2: "Motor 2 details",
    det_status: "Status:",
    det_speed: "Current speed:",
    det_runtime: "Runtime:",
    det_cycles: "Start cycles:",
    page_settings_title: "System settings",
    card_esp_title: "ESP32 settings",
    card_esp_desc: "Network configuration and limits.",
    btn_esp_access: "Device settings access",
    card_ui_title: "Interface settings",
    lbl_theme: "Visual theme"
  }
};

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

    // 4. Setup Event Listeners (ACTUALIZAT PENTRU SINCRONIZARE & UI FIX)
    
    // --- MOTOR 1 TOGGLE ---
    if(document.getElementById('btnM1')) {
        document.getElementById('btnM1').onclick = () => {
            // 1. Setăm timpul comenzii pentru a bloca telemetria
            lastCmdTimeM1 = Date.now();
            
            // 2. Trimitem comanda
            sendCmd({ cmd: 'toggleM1' });
            
            // 3. OPTIMISTIC UI: Schimbăm starea vizuală IMEDIAT
            const badge = document.getElementById('m1_status');
            const isCurrentlyOn = badge.classList.contains('on');
            const newState = !isCurrentlyOn;

            updateStatusBadge('m1_status', newState); 
            
            // Dacă oprim motorul, forțăm și viteza la 0 vizual imediat
            if (!newState) {
                if(document.getElementById('m1_speed')) document.getElementById('m1_speed').textContent = "0";
                if(sliderM1) sliderM1.value = 0;
                if(sliderM1Value) sliderM1Value.textContent = "0";
            }
        };
    }

    // --- MOTOR 2 TOGGLE ---
    if(document.getElementById('btnM2')) {
        document.getElementById('btnM2').onclick = () => {
            lastCmdTimeM2 = Date.now(); // BLOCĂM telemetria pt M2
            
            sendCmd({ cmd: 'toggleM2' });
            
            const badge = document.getElementById('m2_status');
            const isCurrentlyOn = badge.classList.contains('on');
            const newState = !isCurrentlyOn;

            updateStatusBadge('m2_status', newState);

            if (!newState) {
                if(document.getElementById('m2_speed')) document.getElementById('m2_speed').textContent = "0";
                if(sliderM2) sliderM2.value = 0;
                if(sliderM2Value) sliderM2Value.textContent = "0";
            }
        };
    }
    
    // --- ESTOP (URGENȚĂ) ---
    if(document.getElementById('btnEstop')) {
        document.getElementById('btnEstop').onclick = () => {
            lastCmdTimeM1 = Date.now();
            lastCmdTimeM2 = Date.now();
            
            sendCmd({ cmd: 'ESTOP' });
            addAlert(translations[currentLang].btn_estop, 'crit');
            showToast(translations[currentLang].btn_estop, 'error');
            
            // FORȚĂM RESETAREA VIZUALĂ IMEDIATĂ (Viteza 0, Status OFF)
            forceStopUI();
        };
    }

    // Profile Buttons
    if(document.getElementById('btnProfileNormal'))  document.getElementById('btnProfileNormal').onclick  = () => { sendCmd({ cmd: 'set_profile', val: 0 }); updateProfileDesc(0); };
    if(document.getElementById('btnProfileEconomy')) document.getElementById('btnProfileEconomy').onclick = () => { sendCmd({ cmd: 'set_profile', val: 1 }); updateProfileDesc(1); };
    if(document.getElementById('btnProfileSync'))    document.getElementById('btnProfileSync').onclick    = () => { sendCmd({ cmd: 'set_profile', val: 2 }); updateProfileDesc(2); };

    // Slider Logic (Actualizat cu Debounce)
    if (sliderM1) {
        sliderM1.addEventListener("input", () => { 
            lastCmdTimeM1 = Date.now(); // Blocăm serverul în timp ce tragem
            if(sliderM1Value) sliderM1Value.textContent = sliderM1.value; 
        });
        sliderM1.addEventListener("change", () => {
            lastCmdTimeM1 = Date.now(); 
            sendCmd({ cmd: "set_m1_speed", val: parseInt(sliderM1.value) });
        });
    }

    if (sliderM2) {
        sliderM2.addEventListener("input", () => { 
            lastCmdTimeM2 = Date.now();
            if(sliderM2Value) sliderM2Value.textContent = sliderM2.value; 
        });
        sliderM2.addEventListener("change", () => {
            lastCmdTimeM2 = Date.now();
            sendCmd({ cmd: "set_m2_speed", val: parseInt(sliderM2.value) });
        });
    }

    // FFT Controls
    if(btnFftM1) btnFftM1.onclick = () => { fftMode = 'm1'; updateFftVisibility(); };
    if(btnFftM2) btnFftM2.onclick = () => { fftMode = 'm2'; updateFftVisibility(); };
    if(btnFftCompare) btnFftCompare.onclick = () => { fftMode = 'compare'; updateFftVisibility(); };

    // Stats Controls
    btnStatsM1 = document.getElementById('btnStatsM1');
    btnStatsM2 = document.getElementById('btnStatsM2');
    btnStatsCompare = document.getElementById('btnStatsCompare');

    if (btnStatsM1) btnStatsM1.onclick = () => { statsMode = 'm1'; updateStatsVisibility(); };
    if (btnStatsM2) btnStatsM2.onclick = () => { statsMode = 'm2'; updateStatsVisibility(); };
    if (btnStatsCompare) btnStatsCompare.onclick = () => { statsMode = 'compare'; updateStatsVisibility(); };

    updateStatsVisibility();

    // Navigation & Theme
    setupNavAndTheme();
    
    // Initialize Language (Default RO)
    window.setLanguage('ro');

    // 5. Start Connection
    initWebSocket();
});

// ===== Helper Functions =====

// --- FORCE STOP UI (NOU) ---
function forceStopUI() {
    // Setăm etichetele pe OPRIT
    updateStatusBadge('m1_status', false);
    updateStatusBadge('m2_status', false);

    // Setăm textul vitezei pe 0
    if (document.getElementById('m1_speed')) document.getElementById('m1_speed').textContent = "0";
    if (document.getElementById('m2_speed')) document.getElementById('m2_speed').textContent = "0";

    // Resetăm Sliderele
    if (document.getElementById('sliderM1')) document.getElementById('sliderM1').value = 0;
    if (document.getElementById('sliderM2')) document.getElementById('sliderM2').value = 0;
    if (document.getElementById('sliderM1Value')) document.getElementById('sliderM1Value').textContent = "0";
    if (document.getElementById('sliderM2Value')) document.getElementById('sliderM2Value').textContent = "0";
    
    // Opțional: Resetăm și detaliile din pagina Motors
    if(document.getElementById('motor-detail-speed-1')) document.getElementById('motor-detail-speed-1').textContent = "0";
    if(document.getElementById('motor-detail-speed-2')) document.getElementById('motor-detail-speed-2').textContent = "0";
}

// --- LANGUAGE SWITCHER ---
window.setLanguage = function(lang) {
    if (!translations[lang]) return;
    currentLang = lang;

    // Update static elements
    document.querySelectorAll('[data-lang]').forEach(el => {
        const key = el.getAttribute('data-lang');
        if (translations[lang][key]) {
            if(el.tagName === 'INPUT' || el.tagName === 'TEXTAREA') return;
            el.innerHTML = translations[lang][key]; 
        }
    });

    // Highlight vizual steaguri
    document.querySelectorAll('.btn-lang').forEach(btn => btn.classList.remove('active'));
    const activeBtn = document.getElementById(`lang-btn-${lang}`);
    if(activeBtn) activeBtn.classList.add('active');

    // Refresh Dynamic Components
    updateProfileDesc(currentProfileId); 
    
    if (fftChart) {
        fftChart.data.datasets[0].label = translations[lang].motor1;
        fftChart.data.datasets[1].label = translations[lang].motor2;
        fftChart.update();
    }
    
    if (historicalChart) {
        historicalChart.data.datasets[0].label = "RMS (" + translations[lang].motor1 + ")";
        historicalChart.data.datasets[1].label = "RMS (" + translations[lang].motor2 + ")";
        historicalChart.update();
    }
};

function sendCmd(json) {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify(json));
    }
}

function updateProfileDesc(profileId) {
    currentProfileId = profileId; // Save for lang switch
    const descEl = document.getElementById('profileDescriptionText');
    const btns = document.querySelectorAll('.btn-profile');
    btns.forEach(b => b.classList.remove('active'));

    let text = "";
    if (profileId === 0) {
        text = translations[currentLang].prof_desc_normal;
        document.getElementById('btnProfileNormal')?.classList.add('active');
    } else if (profileId === 1) {
        text = translations[currentLang].prof_desc_eco;
        document.getElementById('btnProfileEconomy')?.classList.add('active');
    } else if (profileId === 2) {
        text = translations[currentLang].prof_desc_sync;
        document.getElementById('btnProfileSync')?.classList.add('active');
    } else {
        text = translations[currentLang].prof_select;
    }
    
    if(descEl) descEl.innerHTML = text;
}

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
    status.textContent = translations[currentLang].health_stable;
    status.style.color = baseColor;
  } else if (clamped >= 50) {
    circle.style.stroke = '#FFC107';
    status.textContent = translations[currentLang].health_warning;
    status.style.color = '#FFC107';
  } else {
    circle.style.stroke = '#F44336';
    status.textContent = translations[currentLang].health_critical;
    status.style.color = '#F44336';
  }
}

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

// ===== WebSocket Logic =====
function initWebSocket() {
  const isLocalAP = window.location.hostname === "" || window.location.hostname === "192.168.4.1";
  const wsHost = isLocalAP ? "192.168.4.1" : window.location.hostname;
  const url = `ws://${wsHost}/ws`;

  const statusElem = document.getElementById('connectionStatus');
  
  websocket = new WebSocket(url);

  websocket.onopen = () => {
    if(statusElem) statusElem.innerHTML = '<span style="color: var(--green);">●</span> ' + (currentLang === 'ro' ? 'Conectat' : 'Connected');
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

    // --- 2. MOTOR CONTROL CARDS (ACTUALIZAT CU LOGICA DE TIMEOUT) ---
    
    // UPDATE MOTOR 1 (Doar dacă nu am dat o comandă recent)
    if (Date.now() - lastCmdTimeM1 > CMD_TIMEOUT) {
        updateStatusBadge('m1_status', d.motor1State);
        if (document.getElementById('m1_speed')) document.getElementById('m1_speed').textContent = m1_spd;
        if (document.getElementById('m1_rms')) document.getElementById('m1_rms').textContent = rms1.toFixed(2);
        
        // Slider update (evităm conflictul dacă userul nu interacționează)
        if (sliderM1 && document.activeElement !== sliderM1) {
            sliderM1.value = m1_spd;
            if (sliderM1Value) sliderM1Value.textContent = m1_spd;
        }
    }

    // UPDATE MOTOR 2 (Doar dacă nu am dat o comandă recent)
    if (Date.now() - lastCmdTimeM2 > CMD_TIMEOUT) {
        updateStatusBadge('m2_status', d.motor2State);
        if (document.getElementById('m2_speed')) document.getElementById('m2_speed').textContent = m2_spd;
        if (document.getElementById('m2_rms')) document.getElementById('m2_rms').textContent = rms2.toFixed(2);

        // Slider update
        if (sliderM2 && document.activeElement !== sliderM2) {
            sliderM2.value = m2_spd;
            if (sliderM2Value) sliderM2Value.textContent = m2_spd;
        }
    }

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

    // --- 6. TREND LOGIC ---
    rmsHistory.push(rms1);
    if (rmsHistory.length > 10) rmsHistory.shift();

    rmsHistory2.push(rms2);
    if (rmsHistory2.length > 10) rmsHistory2.shift();

    // Trend M1
    if (rmsHistory.length >= 5) {
        const recent = rmsHistory.slice(-5);
        const older = rmsHistory.slice(0, 5);
        const recentAvg = recent.reduce((a, b) => a + b, 0) / recent.length;
        const olderAvg = older.reduce((a, b) => a + b, 0) / older.length;
        const trendElem = document.getElementById('stat_trend');
        if (trendElem) {
            if (recentAvg > olderAvg + 0.5) trendElem.textContent = translations[currentLang].trend_up;
            else if (recentAvg < olderAvg - 0.5) trendElem.textContent = translations[currentLang].trend_down;
            else trendElem.textContent = translations[currentLang].trend_flat;
        }
    }

    // Trend M2
    if (rmsHistory2.length >= 5) {
        const recent2 = rmsHistory2.slice(-5);
        const older2 = rmsHistory2.slice(0, 5);
        const recentAvg2 = recent2.reduce((a, b) => a + b, 0) / recent2.length;
        const olderAvg2 = older2.reduce((a, b) => a + b, 0) / older2.length;
        const trendElem2 = document.getElementById('stat_trend_m2');
        if (trendElem2) {
            if (recentAvg2 > olderAvg2 + 0.5) trendElem2.textContent = translations[currentLang].trend_up;
            else if (recentAvg2 < olderAvg2 - 0.5) trendElem2.textContent = translations[currentLang].trend_down;
            else trendElem2.textContent = translations[currentLang].trend_flat;
        }
    }

    // --- 7. PROFILE ---
    if (d.profile !== undefined && d.profile !== currentProfileId) {
        updateProfileDesc(d.profile);
    }

    // --- 8. LAST UPDATE ---
    const lastUpdateElem = document.getElementById('lastUpdate');
    if (lastUpdateElem) lastUpdateElem.textContent = new Date().toLocaleTimeString();

    // --- 9. FFT CHARTS ---
    if (fftChart) {
        if (d.fft1 && d.fft1.length > 0) fftChart.data.datasets[0].data = d.fft1;
        if (d.fft2 && d.fft2.length > 0) fftChart.data.datasets[1].data = d.fft2;
        fftChart.update('none');
    }

    // --- 10. HISTORICAL ---
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

    // --- 12. UPDATE PAGES ---
    updateMotorsPage(d);
    updateAlerts(d);
  };

  websocket.onclose = () => {
    if(statusElem) statusElem.innerHTML = '<span style="color: var(--red);">●</span> Disconnected';
    setTimeout(initWebSocket, 2000);
  };
}

function updateStatusBadge(id, isOn) {
    const elem = document.getElementById(id);
    if (!elem) return;
    if (isOn) { 
        elem.textContent = translations[currentLang].status_on; 
        elem.classList.remove("off"); elem.classList.add("on"); 
    } else { 
        elem.textContent = translations[currentLang].status_off; 
        elem.classList.remove("on"); elem.classList.add("off"); 
    }
}

function updateMotorsPage(d) {
    ['1','2'].forEach(i => {
      const isOn = (i === '1') ? d.motor1State : d.motor2State;
      const stEl = document.getElementById(`motor-detail-status-${i}`);
      if(stEl) stEl.textContent = isOn ? translations[currentLang].status_on : translations[currentLang].status_off;
      
      const spEl = document.getElementById(`motor-detail-speed-${i}`);
      if(spEl) spEl.textContent = d[`m${i}_speed`] ?? 0;
    });
}

function addAlert(msg, type) {
    totalAlerts++;
    const li = document.createElement('li');
    li.textContent = `[${new Date().toLocaleTimeString()}] ${msg}`;
    if (type === 'crit') li.style.color = '#ff4444';
    else if (type === 'warn') li.style.color = '#ffbb33';
    
    if (alertsList) {
        alertsList.prepend(li);
        if (alertsList.children.length > 5) alertsList.lastChild.remove();
    }
}

function showToast(msg, type) {
    const container = document.getElementById('toast-container');
    if(!container) return;
    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.textContent = msg;
    container.appendChild(toast);
    setTimeout(() => { toast.remove(); }, 3000);
}

function updateAlerts(d) {
  document.body.style.setProperty('--alert-bg', null);
  
  if (d.alertState === 1) {
    if (lastAlertState !== 1) { 
        addAlert(translations[currentLang].alert_warn_msg, 'warn'); 
        showToast(translations[currentLang].alert_warn_msg, 'warning'); 
    }
    document.body.style.setProperty('--alert-bg', '#2d2d1b');
  } else if (d.alertState === 2) {
    if (lastAlertState !== 2) { 
        addAlert(translations[currentLang].alert_crit_msg, 'crit'); 
        showToast(translations[currentLang].alert_crit_msg, 'error'); 
    }
    document.body.style.setProperty('--alert-bg', '#3b1f1f');
  } else {
    if (lastAlertState !== 0 && lastAlertState !== undefined) {
         addAlert(translations[currentLang].alert_norm_msg, 'info');
    }
  }
  lastAlertState = d.alertState;
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
                    { label: 'RMS (Motor 1)', data: [], borderColor: 'var(--accent-color)', backgroundColor: 'rgba(76, 175, 80, 0.1)', fill: false, tension: 0.4 },
                    { label: 'RMS (Motor 2)', data: [], borderColor: '#FFA500', backgroundColor: 'rgba(255, 165, 0, 0.1)', fill: false, tension: 0.4 }
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
    fftChart.options.plugins.legend.display = false;
  } else if (fftMode === 'm2') {
    fftChart.data.datasets[0].hidden = true;
    fftChart.data.datasets[1].hidden = false;
    if(btnFftM2) btnFftM2.classList.add('active');
    fftChart.options.plugins.legend.display = false;
  } else {
    fftChart.data.datasets[0].hidden = false;
    fftChart.data.datasets[1].hidden = false;
    if(btnFftCompare) btnFftCompare.classList.add('active');
    fftChart.options.plugins.legend.display = true;
  }
  fftChart.update();
}