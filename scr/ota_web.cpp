#include "ota_web.h"

const char otaIndexHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 OTA Update</title>
    <style>
        :root { --primary: #3b82f6; }
        body { font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; margin:0; background:#f8fafc; color:#1e2937; }
        .container { max-width:820px; margin:40px auto; background:white; border-radius:16px; box-shadow:0 10px 15px -3px rgb(0 0 0 / 0.1); overflow:hidden; }
        header { background:var(--primary); color:white; padding:2rem 1.5rem; text-align:center; }
        header h1 { margin:0; font-size:1.75rem; }
        .content { padding:2rem; }
        .info-grid { display:grid; grid-template-columns:repeat(auto-fit, minmax(300px,1fr)); gap:1.5rem; margin-bottom:2rem; }
        .card { background:#f8fafc; border:1px solid #e2e8f0; border-radius:12px; padding:1.5rem; }
        .card h3 { margin:0 0 0.5rem; font-size:1.1rem; color:#64748b; }
        .version { font-size:2.25rem; font-weight:700; margin:0; }
        button { 
            background:var(--primary); color:white; border:none; padding:12px 24px; 
            border-radius:8px; font-weight:600; cursor:pointer; transition:all 0.2s; 
        }
        button:hover { background:#2563eb; transform:translateY(-1px); }
        button:disabled { opacity:0.6; cursor:not-allowed; transform:none; }
        table { width:100%; border-collapse:collapse; margin-top:1rem; }
        th { background:#f1f5f9; text-align:left; padding:14px 12px; font-weight:600; color:#475569; }
        td { padding:14px 12px; border-bottom:1px solid #e2e8f0; }
        .status { padding:1rem; border-radius:8px; margin:1rem 0; font-weight:500; display:none; }
        .green { background:#ecfdf5; color:#10b981; border:1px solid #a7f3d0; }
        .red { background:#fef2f2; color:#ef4444; border:1px solid #fecaca; }
        .blue { background:#dbeafe; color:#1e40af; border:1px solid #bfdbfe; }
    </style>
</head>
<body>
<div class="container">
    <header>
        <h1>🔄 ESP32 OTA Firmware Manager</h1>
    </header>
    <div class="content">
        <div class="info-grid">
            <div class="card">
                <h3>Current Firmware</h3>
                <p id="current-version" class="version">—</p>
            </div>
            <div class="card">
                <h3>Latest Available</h3>
                <p id="latest-version" class="version" style="color:#10b981;">—</p>
            </div>
        </div>

        <button onclick="fetchFirmwareInfo()" id="refresh-btn" style="width:100%; margin-bottom:1.5rem;">
            🔄 Refresh Firmware Information
        </button>

        <div id="status" class="status"></div>

        <h2 style="margin:1.5rem 0 0.75rem;">Available Versions</h2>
        <table>
            <thead>
                <tr>
                    <th>Version</th>
                    <th>Released</th>
                    <th>Size</th>
                    <th>Changelog</th>
                    <th style="width:140px;">Action</th>
                </tr>
            </thead>
            <tbody id="versions-body"></tbody>
        </table>
    </div>
</div>

<script>
async function fetchFirmwareInfo() {
    const btn = document.getElementById('refresh-btn');
    btn.disabled = true;
    btn.textContent = 'Fetching from backend...';

    const status = document.getElementById('status');
    status.style.display = 'none';

    try {
        const res = await fetch('/ota/info');
        const data = await res.json();

        if (data.error) {
            showStatus(data.error, 'red');
            return;
        }

        // Update header info
        document.getElementById('current-version').textContent = data.current_version || '—';
        document.getElementById('latest-version').textContent = data.latest_version || '—';

        // Populate table
        const tbody = document.getElementById('versions-body');
        tbody.innerHTML = '';

        if (data.versions && data.versions.length) {
            data.versions.forEach(v => {
                const isCurrent = v.version === data.current_version;
                const isLatest = v.version === data.latest_version;

                const row = document.createElement('tr');
                row.innerHTML = `
                    <td><strong>${v.version}</strong></td>
                    <td>${v.released_at ? new Date(v.released_at).toLocaleDateString() : '—'}</td>
                    <td>${(v.size_bytes / (1024*1024)).toFixed(1)} MB</td>
                    <td style="max-width:280px; white-space:nowrap; overflow:hidden; text-overflow:ellipsis;">${v.changelog || '—'}</td>
                    <td>
                        ${isCurrent 
                            ? `<button disabled style="background:#10b981;">✅ Installed</button>` 
                            : `<button onclick="installVersion('$$   {v.version}')" style="background:   $${isLatest?'#10b981':'#3b82f6'}">
                                Install${isLatest ? ' Latest' : ''}
                               </button>`}
                    </td>
                `;
                tbody.appendChild(row);
            });
        } else {
            tbody.innerHTML = `<tr><td colspan="5" style="text-align:center; padding:2rem; color:#64748b;">No firmware versions available</td></tr>`;
        }
    } catch (err) {
        showStatus('Failed to reach OTA endpoint: ' + err.message, 'red');
    } finally {
        btn.disabled = false;
        btn.textContent = '🔄 Refresh Firmware Information';
    }
}

async function installVersion(version) {
    if (!confirm(`⚠️ Install firmware v${version}?\n\nThe device will restart automatically.`)) return;

    const status = document.getElementById('status');
    showStatus(`Downloading and flashing v${version}... Do not unplug!`, 'blue');

    try {
        const res = await fetch(`/ota/install?version=${encodeURIComponent(version)}`);
        const data = await res.json();

        if (data.success) {
            showStatus(data.message || 'Update successful — restarting!', 'green');
            setTimeout(() => location.reload(), 4000);
        } else {
            showStatus(data.error || 'Update failed', 'red');
        }
    } catch (err) {
        showStatus('Request failed', 'red');
    }
}

function showStatus(text, type) {
    const el = document.getElementById('status');
    el.textContent = text;
    el.className = `status ${type}`;
    el.style.display = 'block';
}

// Auto-load on page open
window.onload = fetchFirmwareInfo;
</script>
</body>
</html>
)rawliteral";