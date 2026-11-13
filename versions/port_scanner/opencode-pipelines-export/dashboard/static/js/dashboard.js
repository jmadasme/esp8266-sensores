(function () {
    'use strict';

    const sessionsContainer = document.getElementById('sessionsContainer');
    const sessionCountEl = document.getElementById('sessionCount');
    const runningCountEl = document.getElementById('runningCount');
    const completedCountEl = document.getElementById('completedCount');
    const lastUpdateEl = document.getElementById('lastUpdate');

    let knownSessions = new Set();

    function renderStep(step) {
        const statusIcon = step.status === 'completed'
            ? '<i class="bi bi-check-lg"></i> Hecho'
            : step.status === 'in_progress'
                ? '<span class="spinner-border spinner-border-sm" role="status"></span> Ejecutando'
                : step.status === 'error'
                    ? '<i class="bi bi-exclamation-triangle"></i> Error'
                    : '<i class="bi bi-hourglass"></i> Pendiente';

        return `
            <div class="step-item ${step.active ? 'active' : ''} step-${step.status}"
                 data-step="${step.key}" data-status="${step.status}">
                <div class="step-indicator">
                    <div class="step-icon-wrapper">
                        <i class="${step.icon}"></i>
                    </div>
                    <div class="step-connector"></div>
                </div>
                <div class="step-content">
                    <div class="step-header">
                        <span class="step-label">${step.label}</span>
                        <span class="step-status-badge status-${step.status}">${statusIcon}</span>
                    </div>
                    ${step.iterations > 0 ? `
                        <div class="step-iterations">
                            <i class="bi bi-arrow-repeat"></i> Iteraciones: <strong>${step.iterations}</strong>
                        </div>` : ''}
                    ${step.comment ? `
                        <div class="step-comment">${step.comment}</div>` : ''}
                </div>
            </div>
        `;
    }

    function renderSession(session) {
        const dotClass = session.completed ? 'completed'
            : session.estado === 'corriendo' || session.progress > 0 ? 'running'
            : session.estado === 'error' ? 'error' : 'pending';

        const statusTag = `<span class="status-tag tag-${session.estado}">${session.estado}</span>`;
        const completionBadge = session.completed
            ? '<span class="completion-badge"><i class="bi bi-check2-circle"></i> Completado</span>' : '';
        const pidBadge = session.pid
            ? `<span class="pid-badge"><i class="bi bi-hdd"></i> PID ${session.pid}</span>` : '';
        const specHtml = session.spec
            ? `<div class="session-spec">
                <i class="bi bi-card-text"></i>
                <span class="spec-text">${session.spec.substring(0, 120)}${session.spec.length > 120 ? '...' : ''}</span>
               </div>` : '';
        const logHtml = session.log_preview
            ? `<div class="session-log">
                <div class="log-header"><i class="bi bi-terminal"></i> Log</div>
                <pre class="log-content">${session.log_preview}</pre>
               </div>` : '';
        const stepsHtml = (session.steps || []).map(renderStep).join('');
        const currentStepLabel = session.current_step
            ? `<i class="bi bi-arrow-right-circle"></i> Paso actual: <strong>${session.current_step}</strong>`
            : 'Progreso general';

        return `
            <div class="session-card" data-session="${session.alias}" data-progress="${session.progress}" data-completed="${session.completed}">
                <div class="session-header">
                    <div class="session-title-section">
                        <span class="session-status-dot ${dotClass}"></span>
                        <h3 class="session-name">${session.alias}</h3>
                        ${statusTag}
                        ${completionBadge}
                        ${pidBadge}
                    </div>
                    <div class="session-meta">
                        <span class="meta-item"><i class="bi bi-clock"></i> ${session.updated_at || session.inicio || '—'}</span>
                        <span class="meta-item"><i class="bi bi-percent"></i> ${session.progress}%</span>
                    </div>
                </div>
                <div class="progress-section">
                    <div class="progress-header">
                        <span class="progress-label">${currentStepLabel}</span>
                        <span class="progress-percentage">${session.progress}%</span>
                    </div>
                    <div class="progress-bar-track">
                        <div class="progress-bar-fill" style="width: ${session.progress}%"></div>
                    </div>
                </div>
                ${specHtml}
                <div class="steps-container">${stepsHtml}</div>
                ${logHtml}
            </div>
        `;
    }

    function updateDashboard(data) {
        const sessions = data.sessions || [];
        const newNames = new Set(sessions.map(s => s.alias));
        let html = '';
        let running = 0;
        let completed = 0;

        if (sessions.length === 0) {
            html = `
                <div class="empty-state">
                    <i class="bi bi-cpu"></i>
                    <h3>No hay sesiones activas</h3>
                    <p>Las sesiones opencode aparecerán aquí cuando el orquestrador las inicie.</p>
                    <div class="empty-hint">
                        <i class="bi bi-lightbulb"></i>
                        Envía un comando al <strong>orquestrador</strong> con el formato:
                        <code>@analista &lt;alias&gt;, "&lt;instruccion&gt;"</code>
                    </div>
                </div>`;
        } else {
            sessions.forEach(s => {
                if (s.completed) completed++;
                else if (s.estado === 'corriendo' || s.progress > 0) running++;
                html += renderSession(s);
            });
        }

        sessionsContainer.innerHTML = html;
        sessionCountEl.textContent = sessions.length;
        runningCountEl.textContent = running;
        completedCountEl.textContent = completed;
        lastUpdateEl.textContent = 'Última actualización: ' + new Date().toLocaleTimeString();

        knownSessions = newNames;
    }

    function connectSSE() {
        const eventSource = new EventSource('/stream');

        eventSource.onmessage = function (e) {
            try {
                const data = JSON.parse(e.data);
                if (data && data.sessions) {
                    updateDashboard(data);
                }
            } catch (err) {
                console.warn('SSE parse error:', err);
            }
        };

        eventSource.onerror = function () {
            console.warn('SSE connection lost, retrying in 3s...');
            eventSource.close();
            setTimeout(connectSSE, 3000);
        };
    }

    connectSSE();
})();
