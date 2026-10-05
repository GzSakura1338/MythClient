// State management
let currentTab = "dashboard";
let allUsers = [];
let selectedKeyIds = [];
let logsPage = 1;
const logsLimit = 15;
let logsTotalPages = 1;
let autoRefreshTimer = null;

// Initialization
document.addEventListener("DOMContentLoaded", () => {
    initTabs();
    initOnlineUsersModal();
    refreshAllData();
    startAutoRefresh();
});

// Toast system
function showToast(message, type = "success") {
    const container = document.getElementById("toast-container");
    if (!container) return;

    const toast = document.createElement("div");
    toast.className = `toast toast-${type}`;
    toast.textContent = message;
    container.appendChild(toast);

    // Trigger reflow for transition
    toast.offsetHeight;
    toast.classList.add("show");

    setTimeout(() => {
        toast.classList.remove("show");
        setTimeout(() => toast.remove(), 300);
    }, 3000);
}

// Global API Fetch helper
async function apiRequest(url, method = "GET", bodyData = null, isJson = false) {
    try {
        const options = { method };
        
        if (bodyData) {
            if (isJson) {
                options.headers = { "Content-Type": "application/json" };
                options.body = JSON.stringify(bodyData);
            } else {
                options.headers = { "Content-Type": "application/x-www-form-urlencoded" };
                const params = new URLSearchParams();
                for (const key in bodyData) {
                    if (bodyData[key] !== null && bodyData[key] !== undefined) {
                        params.append(key, bodyData[key]);
                    }
                }
                options.body = params.toString();
            }
        }

        const response = await fetch(url, options);
        
        if (response.status === 401) {
            showToast("登录过期或未授权，即将重定向...", "error");
            setTimeout(() => window.location.href = "/admin/login", 1500);
            throw new Error("Unauthorized");
        }

        if (!response.ok) {
            const errText = await response.text();
            throw new Error(errText || `请求失败 (${response.status})`);
        }

        // Check if content-type is json
        const contentType = response.headers.get("content-type");
        if (contentType && contentType.includes("application/json")) {
            return await response.json();
        }
        return await response.text();
    } catch (error) {
        if (error.message !== "Unauthorized") {
            showToast(error.message, "error");
        }
        throw error;
    }
}

// Tabs Switching
function initTabs() {
    const navItems = document.querySelectorAll(".nav-item");
    navItems.forEach(item => {
        item.addEventListener("click", () => {
            const tabName = item.getAttribute("data-tab");
            if (tabName === currentTab) return;

            // Update UI
            navItems.forEach(i => i.classList.remove("active"));
            item.classList.add("active");

            document.querySelectorAll(".tab-panel").forEach(panel => panel.classList.remove("active"));
            document.getElementById(`tab-${tabName}`).classList.add("active");

            currentTab = tabName;
            loadTabSpecificData(tabName);
        });
    });
}

function loadTabSpecificData(tabName) {
    switch (tabName) {
        case "dashboard":
            refreshDashboard();
            break;
        case "users":
            loadUsers();
            break;
        case "keys":
            loadKeys();
            break;
        case "blacklist":
            loadBlacklist();
            break;
        case "settings":
            loadSettings();
            break;
        case "logs":
            loadAuditLogs();
            break;
    }
}

// Auto Refresh
function startAutoRefresh() {
    if (autoRefreshTimer) clearInterval(autoRefreshTimer);
    autoRefreshTimer = setInterval(() => {
        const indicator = document.getElementById("poll-indicator");
        if (indicator) indicator.textContent = "正在拉取最新数据...";
        
        refreshAllData().finally(() => {
            if (indicator) {
                const now = new Date();
                const timeStr = now.toTimeString().split(" ")[0];
                indicator.textContent = `上次更新: ${timeStr}`;
            }
        });
    }, 3000); // 30 seconds
}

async function refreshAllData() {
    try {
        // Always refresh active session online counter in header
        const stats = await apiRequest("/admin/api/stats");
        document.getElementById("header-online-count").textContent = stats.online;

        // Load data specific to current tab
        if (currentTab === "dashboard") {
            updateDashboardDOM(stats);
        } else if (currentTab === "users") {
            await loadUsers();
        } else if (currentTab === "keys") {
            await loadKeys();
        } else if (currentTab === "blacklist") {
            await loadBlacklist();
        }
    } catch (e) {
        // Silence or let API helper notify via toast
    }
}

// Dashboard Functions
async function refreshDashboard() {
    try {
        const stats = await apiRequest("/admin/api/stats");
        updateDashboardDOM(stats);
        document.getElementById("header-online-count").textContent = stats.online;
        showToast("仪表盘数据已刷新");
    } catch (e) {}
}

function updateDashboardDOM(stats) {
    document.getElementById("stat-users").textContent = stats.users;
    document.getElementById("stat-total-keys").textContent = stats.totalKeys;
    document.getElementById("stat-active-keys").textContent = stats.activeKeys;
    document.getElementById("stat-online").textContent = stats.online;
}

async function sendBroadcast(event) {
    event.preventDefault();
    const msgInput = document.getElementById("broadcast-msg");
    const message = msgInput.value.trim();
    if (!message) return;

    try {
        const res = await apiRequest("/admin/api/broadcast", "POST", { message });
        showToast(res || "广播消息发送成功");
        msgInput.value = "";
    } catch (e) {}
}

// Online Users Modal
function initOnlineUsersModal() {
    document.getElementById("online-counter-btn").addEventListener("click", async () => {
        try {
            const list = await apiRequest("/admin/api/online");
            const ul = document.getElementById("online-users-list");
            ul.innerHTML = "";
            
            if (list.length === 0) {
                ul.innerHTML = `<li style="text-align: center; color: var(--color-body); padding: 12px 0;">当前暂无在线会话</li>`;
            } else {
                list.forEach(username => {
                    const li = document.createElement("li");
                    li.style.display = "flex";
                    li.style.alignItems = "center";
                    li.style.gap = "8px";
                    li.style.padding = "8px 12px";
                    li.style.backgroundColor = "var(--color-surface-soft)";
                    li.style.borderRadius = "6px";
                    li.style.fontSize = "14px";
                    li.innerHTML = `
                        <span class="online-pulse"></span>
                        <strong class="code-cell">${username}</strong>
                    `;
                    ul.appendChild(li);
                });
            }
            openModal("online-users-modal");
        } catch (e) {}
    });
}

// Users Tab Functions
async function loadUsers() {
    try {
        allUsers = await apiRequest("/admin/api/users");
        filterUsers();
    } catch (e) {}
}

function filterUsers() {
    const query = document.getElementById("user-search").value.trim().toLowerCase();
    const tbody = document.getElementById("users-tbody");
    tbody.innerHTML = "";

    const filtered = allUsers.filter(u => 
        u.username.toLowerCase().includes(query) || 
        (u.qq && u.qq.toLowerCase().includes(query))
    );

    if (filtered.length === 0) {
        tbody.innerHTML = `<tr><td colspan="8" style="text-align: center; color: var(--color-body);">未找到匹配的用户数据</td></tr>`;
        return;
    }

    filtered.forEach(u => {
        const tr = document.createElement("tr");

        let statusBadge = `<span class="badge badge-success">正常</span>`;
        let isBanned = false;
        
        if (u.banned) {
            statusBadge = `<span class="badge badge-danger">永久封禁</span>`;
            isBanned = true;
        } else if (u.banExpiryDate > 0) {
            if (u.banExpiryDate > Date.now()) {
                statusBadge = `<span class="badge badge-warning">临时封禁</span>`;
                isBanned = true;
            }
        }

        const expiryStr = u.expiryDate > 0 ? formatDate(u.expiryDate) : "暂无订阅";
        const banExpiryStr = isBanned ? (u.banExpiryDate > 0 ? formatDate(u.banExpiryDate) : "永久") : "-";

        tr.innerHTML = `
            <td>${u.id}</td>
            <td style="font-weight: 600;">${u.username}</td>
            <td>${u.qq || "-"}</td>
            <td><span class="code-cell" style="max-width: 140px; display: inline-block; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;">${u.hwid || "-"}</span></td>
            <td>${expiryStr}</td>
            <td>${statusBadge}</td>
            <td>${banExpiryStr}</td>
            <td style="text-align: right; white-space: nowrap;">
                <div style="display: flex; gap: 6px; justify-content: flex-end;">
                    <button class="btn btn-secondary" style="height: 28px; padding: 0 10px; font-size: 12px;" onclick="openBanModal(${u.id})">封禁设置</button>
                    <button class="btn btn-secondary" style="height: 28px; padding: 0 10px; font-size: 12px;" onclick="openPasswordModal(${u.id})">改密</button>
                    <button class="btn btn-secondary" style="height: 28px; padding: 0 10px; font-size: 12px;" onclick="resetHwid(${u.id}, '${u.username}')">重置HWID</button>
                    <button class="btn btn-danger" style="height: 28px; padding: 0 10px; font-size: 12px;" onclick="kickUser(${u.id}, '${u.username}')">踢线</button>
                </div>
            </td>
        `;
        tbody.appendChild(tr);
    });
}

function openBanModal(userId) {
    document.getElementById("ban-user-id").value = userId;
    document.getElementById("ban-duration").value = "0";
    openModal("user-ban-modal");
}

async function submitUserBan(event) {
    event.preventDefault();
    const id = document.getElementById("ban-user-id").value;
    const duration = document.getElementById("ban-duration").value;

    try {
        const res = await apiRequest(`/admin/api/users/${id}/ban`, "POST", { duration });
        showToast(res || "用户封禁状态已更新");
        closeModal("user-ban-modal");
        loadUsers();
    } catch (e) {}
}

function openPasswordModal(userId) {
    document.getElementById("password-user-id").value = userId;
    document.getElementById("new-user-password").value = "";
    openModal("user-password-modal");
}

async function submitResetPassword(event) {
    event.preventDefault();
    const id = document.getElementById("password-user-id").value;
    const password = document.getElementById("new-user-password").value.trim();
    if (!password) return;

    try {
        const res = await apiRequest(`/admin/api/users/${id}/reset-password`, "POST", { password });
        showToast(res || "密码已重置");
        closeModal("user-password-modal");
    } catch (e) {}
}

async function resetHwid(id, username) {
    if (!confirm(`确认要清除用户 [${username}] 的 HWID 硬件绑定吗？这将强制其重新登录绑定。`)) return;

    try {
        const res = await apiRequest(`/admin/api/users/${id}/reset-hwid`, "POST");
        showToast(res || "硬件绑定已成功重置");
        loadUsers();
    } catch (e) {}
}

async function kickUser(id, username) {
    if (!confirm(`确认要将在线用户 [${username}] 强制下线吗？`)) return;

    try {
        const res = await apiRequest(`/admin/api/users/${id}/kick`, "POST");
        showToast(res || "用户强制下线指令已执行");
        loadUsers();
    } catch (e) {}
}

// Keys Tab Functions
async function loadKeys() {
    try {
        const keys = await apiRequest("/admin/api/keys");
        const tbody = document.getElementById("keys-tbody");
        tbody.innerHTML = "";
        selectedKeyIds = [];
        document.getElementById("delete-selected-keys-btn").disabled = true;
        document.getElementById("keys-select-all").checked = false;

        if (keys.length === 0) {
            tbody.innerHTML = `<tr><td colspan="7" style="text-align: center; color: var(--color-body);">暂无任何卡密数据</td></tr>`;
            return;
        }

        keys.forEach(k => {
            const tr = document.createElement("tr");
            const durationDays = (k.duration / (24 * 3600 * 1000)).toFixed(1);
            
            tr.innerHTML = `
                <td><input type="checkbox" class="key-checkbox" value="${k.id}" onclick="onKeySelectToggle(this)"></td>
                <td>${k.id}</td>
                <td><span class="code-cell">${k.content}</span></td>
                <td>${durationDays} 天</td>
                <td>${k.used ? '<span class="badge badge-danger">已使用</span>' : '<span class="badge badge-success">未使用</span>'}</td>
                <td>${k.usedBy ? k.usedBy.username : "-"}</td>
                <td>${k.remark || "-"}</td>
            `;
            tbody.appendChild(tr);
        });
    } catch (e) {}
}

function toggleSelectAllKeys(source) {
    const checkboxes = document.querySelectorAll(".key-checkbox");
    selectedKeyIds = [];
    checkboxes.forEach(c => {
        c.checked = source.checked;
        if (source.checked) {
            selectedKeyIds.push(parseInt(c.value));
        }
    });
    updateDeleteKeysButton();
}

function onKeySelectToggle(checkbox) {
    const id = parseInt(checkbox.value);
    if (checkbox.checked) {
        selectedKeyIds.push(id);
    } else {
        selectedKeyIds = selectedKeyIds.filter(val => val !== id);
        document.getElementById("keys-select-all").checked = false;
    }
    updateDeleteKeysButton();
}

function updateDeleteKeysButton() {
    const btn = document.getElementById("delete-selected-keys-btn");
    btn.disabled = selectedKeyIds.length === 0;
    btn.textContent = selectedKeyIds.length > 0 ? `删除选中 (${selectedKeyIds.length})` : "删除选中";
}

async function deleteSelectedKeys() {
    if (selectedKeyIds.length === 0) return;
    if (!confirm(`确认要永久删除选中的 ${selectedKeyIds.length} 个卡密吗？`)) return;

    try {
        const res = await apiRequest("/admin/api/keys/delete", "POST", selectedKeyIds, true);
        showToast(res || "选中的卡密已删除");
        loadKeys();
    } catch (e) {}
}

async function generateKeys(event) {
    event.preventDefault();
    const amount = document.getElementById("gen-amount").value;
    const duration = document.getElementById("gen-duration").value;
    const remark = document.getElementById("gen-remark").value.trim();
    const downloadChecked = document.getElementById("gen-download").checked;

    if (downloadChecked) {
        // Use form submission for attachment downloading trigger
        const form = document.createElement("form");
        form.method = "POST";
        form.action = `/admin/api/keys/generate?download=true`;
        form.style.display = "none";

        const params = { amount, duration, remark };
        for (const k in params) {
            const input = document.createElement("input");
            input.name = k;
            input.value = params[k];
            form.appendChild(input);
        }

        document.body.appendChild(form);
        form.submit();
        document.body.removeChild(form);
        
        showToast("正在生成并开始下载文本文件...");
        setTimeout(() => loadKeys(), 1000);
    } else {
        try {
            const res = await apiRequest("/admin/api/keys/generate", "POST", { amount, duration, remark });
            showToast(res || "生成卡密成功");
            document.getElementById("gen-remark").value = "";
            loadKeys();
        } catch (e) {}
    }
}

// Blacklist Functions
async function loadBlacklist() {
    try {
        const list = await apiRequest("/admin/api/blacklist");
        const tbody = document.getElementById("blacklist-tbody");
        tbody.innerHTML = "";

        if (list.length === 0) {
            tbody.innerHTML = `<tr><td colspan="4" style="text-align: center; color: var(--color-body);">物理黑名单规则为空</td></tr>`;
            return;
        }

        list.forEach(b => {
            const tr = document.createElement("tr");
            tr.innerHTML = `
                <td>${b.id}</td>
                <td><span class="code-cell">${b.hwid}</span></td>
                <td>${b.reason || "手动添加封禁"}</td>
                <td style="text-align: right;">
                    <button class="btn btn-danger" style="height: 28px; padding: 0 12px; font-size: 12px;" onclick="removeBlacklist(${b.id})">解除黑名单</button>
                </td>
            `;
            tbody.appendChild(tr);
        });
    } catch (e) {}
}

function openAddBlacklistModal() {
    document.getElementById("add-black-hwid").value = "";
    document.getElementById("add-black-reason").value = "";
    openModal("blacklist-add-modal");
}

async function submitAddBlacklist(event) {
    event.preventDefault();
    const hwid = document.getElementById("add-black-hwid").value.trim();
    const reason = document.getElementById("add-black-reason").value.trim();
    if (!hwid) return;

    try {
        const res = await apiRequest("/admin/api/blacklist", "POST", { hwid, reason });
        showToast(res || "硬件已成功锁定至黑名单");
        closeModal("blacklist-add-modal");
        loadBlacklist();
    } catch (e) {}
}

async function removeBlacklist(id) {
    if (!confirm("确认要移除该硬件黑名单规则吗？")) return;

    try {
        const res = await apiRequest(`/admin/api/blacklist/${id}`, "DELETE");
        showToast(res || "硬件已被移除黑名单");
        loadBlacklist();
    } catch (e) {}
}

// Settings Functions
async function loadSettings() {
    try {
        const map = await apiRequest("/admin/api/settings");
        document.getElementById("set-motd").value = map.motd || "";
        document.getElementById("set-version").value = map.min_version || "";
    } catch (e) {}
}

async function saveSettings(event) {
    event.preventDefault();
    const motd = document.getElementById("set-motd").value.trim();
    const min_version = document.getElementById("set-version").value.trim();

    try {
        const res = await apiRequest("/admin/api/settings", "POST", { motd, min_version });
        showToast(res || "系统配置保存成功");
        loadSettings();
    } catch (e) {}
}

// Audit Logs Functions
async function loadAuditLogs() {
    try {
        const res = await apiRequest(`/admin/api/logs?page=${logsPage}&limit=${logsLimit}`);
        const tbody = document.getElementById("logs-tbody");
        tbody.innerHTML = "";

        logsTotalPages = res.pages;
        updateLogsPaginationDOM(res.total, res.page, res.data.length);

        if (res.data.length === 0) {
            tbody.innerHTML = `<tr><td colspan="7" style="text-align: center; color: var(--color-body);">没有任何审计操作日志记录</td></tr>`;
            return;
        }

        res.data.forEach(log => {
            const tr = document.createElement("tr");
            tr.innerHTML = `
                <td>${log.id}</td>
                <td style="font-weight: 600;">${log.operator}</td>
                <td><span class="badge ${getLogActionBadgeClass(log.action)}">${log.action}</span></td>
                <td>${log.target ? `<span class="code-cell">${log.target}</span>` : "-"}</td>
                <td style="font-size: 13px;">${log.details || "-"}</td>
                <td><span class="code-cell">${log.ip}</span></td>
                <td style="white-space: nowrap;">${formatDate(log.timestamp)}</td>
            `;
            tbody.appendChild(tr);
        });
    } catch (e) {}
}

function getLogActionBadgeClass(action) {
    if (action.includes("BAN") || action.includes("DEL") || action.includes("KICK")) {
        return "badge-danger";
    }
    if (action.includes("GEN") || action.includes("ADD") || action.includes("UPDATE")) {
        return "badge-success";
    }
    return "badge-warning";
}

function updateLogsPaginationDOM(total, page, count) {
    const info = document.getElementById("logs-page-info");
    const start = total === 0 ? 0 : (page - 1) * logsLimit + 1;
    const end = start + count - 1;

    info.textContent = `显示第 ${start} 到 ${end} 条，共 ${total} 条`;

    document.getElementById("logs-prev-btn").disabled = page <= 1;
    document.getElementById("logs-next-btn").disabled = page >= logsTotalPages;
}

function prevLogsPage() {
    if (logsPage > 1) {
        logsPage--;
        loadAuditLogs();
    }
}

function nextLogsPage() {
    if (logsPage < logsTotalPages) {
        logsPage++;
        loadAuditLogs();
    }
}

// Modal actions helpers
function openModal(id) {
    const modal = document.getElementById(id);
    if (modal) modal.classList.add("active");
}

function closeModal(id) {
    const modal = document.getElementById(id);
    if (modal) modal.classList.remove("active");
}

// Common Date Helper
function formatDate(timestamp) {
    if (!timestamp) return "-";
    const date = new Date(timestamp);
    const y = date.getFullYear();
    const m = String(date.getMonth() + 1).padStart(2, "0");
    const d = String(date.getDate()).padStart(2, "0");
    const h = String(date.getHours()).padStart(2, "0");
    const min = String(date.getMinutes()).padStart(2, "0");
    const s = String(date.getSeconds()).padStart(2, "0");
    return `${y}-${m}-${d} ${h}:${min}:${s}`;
}
