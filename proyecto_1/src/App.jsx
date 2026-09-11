import React, { useState, useEffect, useRef } from 'react';

export default function App() {
  const [inputCode, setInputCode] = useState('');
  const [output, setOutput] = useState('');
  const [autoConfirm, setAutoConfirm] = useState(false);
  const [loading, setLoading] = useState(false);
  const [healthStatus, setHealthStatus] = useState('checking');
  const fileInputRef = useRef(null);

  // Check health on mount and periodically
  useEffect(() => {
    checkHealth();
    const interval = setInterval(checkHealth, 10000);
    return () => clearInterval(interval);
  }, []);

  const checkHealth = async () => {
    try {
      const res = await fetch('http://localhost:8080/api/health');
      if (res.ok) {
        const text = await res.text();
        if (text.trim() === 'ok') {
          setHealthStatus('online');
          return;
        }
      }
      setHealthStatus('offline');
    } catch (err) {
      setHealthStatus('offline');
    }
  };

  const handleFileChange = (e) => {
    const file = e.target.files[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (event) => {
      setInputCode(event.target.result || '');
    };
    reader.readAsText(file);
  };

  const handleExecute = async () => {
    if (!inputCode.trim()) {
      setOutput('Error: El área de entrada está vacía. Ingrese comandos o cargue un archivo .smia.');
      return;
    }

    setLoading(true);
    setOutput('Ejecutando comandos en el backend C++...');

    const url = `http://localhost:8080/api/execute?confirm=${autoConfirm ? 'true' : 'false'}`;

    try {
      const response = await fetch(url, {
        method: 'POST',
        headers: {
          'Content-Type': 'text/plain; charset=utf-8',
        },
        body: inputCode,
      });

      if (!response.ok) {
        throw new Error(`Respuesta HTTP de error: ${response.status} ${response.statusText}`);
      }

      const resultText = await response.text();
      setOutput(resultText || 'Ejecución completada sin salida.');
    } catch (error) {
      setOutput(`Error de conexión con la API backend:\n${error.message}\n\nAsegúrese de que el servidor C++ (run_api) esté ejecutándose en http://localhost:8080`);
    } finally {
      setLoading(false);
    }
  };

  const handleClear = () => {
    setInputCode('');
    setOutput('');
    if (fileInputRef.current) {
      fileInputRef.current.value = '';
    }
  };

  return (
    <div style={styles.container}>
      {/* Header */}
      <header style={styles.header}>
        <div style={styles.headerLeft}>
          <div style={styles.logoIcon}>⚡</div>
          <div>
            <h1 style={styles.title}>MIA File System API — Fase 6</h1>
            <p style={styles.subtitle}>Interfaz React HTTP para Backend C++</p>
          </div>
        </div>
        <div style={styles.headerRight}>
          <div style={styles.statusBadge(healthStatus)}>
            <span style={styles.statusDot(healthStatus)}></span>
            API: {healthStatus === 'online' ? 'Conectado (8080)' : healthStatus === 'checking' ? 'Verificando...' : 'Desconectado'}
          </div>
        </div>
      </header>

      {/* Main Workspace */}
      <main style={styles.main}>
        {/* Controls Bar */}
        <div style={styles.toolbar}>
          <div style={styles.toolbarGroup}>
            <label style={styles.fileLabel}>
              📁 Seleccionar archivo .smia
              <input
                ref={fileInputRef}
                type="file"
                accept=".smia,.txt"
                onChange={handleFileChange}
                style={styles.fileInput}
              />
            </label>

            <label style={styles.checkboxLabel}>
              <input
                type="checkbox"
                checked={autoConfirm}
                onChange={(e) => setAutoConfirm(e.target.checked)}
                style={styles.checkbox}
              />
              Confirmaciones automáticas (confirm={autoConfirm ? 'true' : 'false'})
            </label>
          </div>

          <div style={styles.toolbarGroup}>
            <button
              onClick={handleClear}
              disabled={loading}
              style={styles.clearBtn}
            >
              🗑️ Limpiar
            </button>
            <button
              onClick={handleExecute}
              disabled={loading}
              style={loading ? styles.executeBtnDisabled : styles.executeBtn}
            >
              {loading ? '⏳ Ejecutando...' : '▶️ Ejecutar'}
            </button>
          </div>
        </div>

        {/* Dual Editor Grid */}
        <div style={styles.grid}>
          {/* Input Area */}
          <div style={styles.panel}>
            <div style={styles.panelHeader}>
              <span>📝 Comandos de Entrada (.smia)</span>
              <span style={styles.charCount}>{inputCode.length} caracteres</span>
            </div>
            <textarea
              value={inputCode}
              onChange={(e) => setInputCode(e.target.value)}
              placeholder="Ingrese comandos aquí o seleccione un archivo .smia...&#10;&#10;Ejemplo:&#10;mkdisk -size=10 -unit=M -path=DiscoAPI.mia&#10;fdisk -size=2 -unit=M -path=DiscoAPI.mia -name=P1&#10;mount -path=DiscoAPI.mia -name=P1"
              style={styles.textareaInput}
            />
          </div>

          {/* Output Area */}
          <div style={styles.panel}>
            <div style={styles.panelHeader}>
              <span>💻 Salida de Consola (Backend C++)</span>
              {output && (
                <button
                  onClick={() => navigator.clipboard.writeText(output)}
                  style={styles.copyBtn}
                >
                  📋 Copiar
                </button>
              )}
            </div>
            <textarea
              value={output}
              readOnly
              placeholder="La respuesta del backend C++ aparecerá aquí..."
              style={styles.textareaOutput}
            />
          </div>
        </div>
      </main>

      {/* Footer */}
      <footer style={styles.footer}>
        MIA 2026 — Proyecto 1 Fase 6 | Backend C++ cpp-httplib | React + HTTP
      </footer>
    </div>
  );
}

const styles = {
  container: {
    fontFamily: '"Segoe UI", Roboto, Helvetica, Arial, sans-serif',
    backgroundColor: '#0f172a',
    color: '#f8fafc',
    minHeight: '100vh',
    display: 'flex',
    flexDirection: 'column',
  },
  header: {
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: '1rem 2rem',
    backgroundColor: '#1e293b',
    borderBottom: '1px solid #334155',
  },
  headerLeft: {
    display: 'flex',
    alignItems: 'center',
    gap: '1rem',
  },
  logoIcon: {
    fontSize: '2rem',
    background: 'linear-gradient(135deg, #3b82f6, #8b5cf6)',
    borderRadius: '12px',
    padding: '0.4rem 0.6rem',
  },
  title: {
    margin: 0,
    fontSize: '1.4rem',
    fontWeight: '700',
    color: '#f8fafc',
  },
  subtitle: {
    margin: 0,
    fontSize: '0.85rem',
    color: '#94a3b8',
  },
  headerRight: {
    display: 'flex',
    alignItems: 'center',
  },
  statusBadge: (status) => ({
    display: 'flex',
    alignItems: 'center',
    gap: '0.5rem',
    padding: '0.4rem 0.9rem',
    borderRadius: '20px',
    fontSize: '0.85rem',
    fontWeight: '600',
    backgroundColor: status === 'online' ? 'rgba(34, 197, 94, 0.15)' : 'rgba(239, 68, 68, 0.15)',
    color: status === 'online' ? '#4ade80' : '#f87171',
    border: `1px solid ${status === 'online' ? '#22c55e' : '#ef4444'}`,
  }),
  statusDot: (status) => ({
    width: '8px',
    height: '8px',
    borderRadius: '50%',
    backgroundColor: status === 'online' ? '#22c55e' : '#ef4444',
  }),
  main: {
    flex: 1,
    padding: '1.5rem 2rem',
    display: 'flex',
    flexDirection: 'column',
    gap: '1rem',
  },
  toolbar: {
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#1e293b',
    padding: '0.8rem 1.2rem',
    borderRadius: '10px',
    border: '1px solid #334155',
    flexWrap: 'wrap',
    gap: '1rem',
  },
  toolbarGroup: {
    display: 'flex',
    alignItems: 'center',
    gap: '1.2rem',
  },
  fileLabel: {
    backgroundColor: '#3b82f6',
    color: '#ffffff',
    padding: '0.5rem 1rem',
    borderRadius: '6px',
    cursor: 'pointer',
    fontWeight: '600',
    fontSize: '0.9rem',
    transition: 'background-color 0.2s',
  },
  fileInput: {
    display: 'none',
  },
  checkboxLabel: {
    display: 'flex',
    alignItems: 'center',
    gap: '0.5rem',
    fontSize: '0.9rem',
    color: '#cbd5e1',
    cursor: 'pointer',
  },
  checkbox: {
    width: '16px',
    height: '16px',
    accentColor: '#3b82f6',
    cursor: 'pointer',
  },
  clearBtn: {
    backgroundColor: '#475569',
    color: '#ffffff',
    border: 'none',
    padding: '0.5rem 1.2rem',
    borderRadius: '6px',
    cursor: 'pointer',
    fontWeight: '600',
    fontSize: '0.9rem',
  },
  executeBtn: {
    backgroundColor: '#22c55e',
    color: '#ffffff',
    border: 'none',
    padding: '0.5rem 1.4rem',
    borderRadius: '6px',
    cursor: 'pointer',
    fontWeight: '700',
    fontSize: '0.9rem',
    boxShadow: '0 4px 12px rgba(34, 197, 94, 0.3)',
  },
  executeBtnDisabled: {
    backgroundColor: '#334155',
    color: '#94a3b8',
    border: 'none',
    padding: '0.5rem 1.4rem',
    borderRadius: '6px',
    cursor: 'not-allowed',
    fontWeight: '700',
    fontSize: '0.9rem',
  },
  grid: {
    display: 'grid',
    gridTemplateColumns: '1fr 1fr',
    gap: '1.5rem',
    flex: 1,
    minHeight: '500px',
  },
  panel: {
    backgroundColor: '#1e293b',
    borderRadius: '10px',
    border: '1px solid #334155',
    display: 'flex',
    flexDirection: 'column',
    overflow: 'hidden',
  },
  panelHeader: {
    padding: '0.75rem 1rem',
    backgroundColor: '#0f172a',
    borderBottom: '1px solid #334155',
    fontWeight: '600',
    fontSize: '0.9rem',
    color: '#cbd5e1',
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  charCount: {
    fontSize: '0.8rem',
    color: '#64748b',
  },
  copyBtn: {
    backgroundColor: '#334155',
    color: '#f8fafc',
    border: 'none',
    padding: '0.2rem 0.6rem',
    borderRadius: '4px',
    cursor: 'pointer',
    fontSize: '0.75rem',
  },
  textareaInput: {
    flex: 1,
    backgroundColor: '#0f172a',
    color: '#38bdf8',
    border: 'none',
    padding: '1rem',
    fontFamily: '"Fira Code", "Courier New", monospace',
    fontSize: '0.95rem',
    resize: 'none',
    outline: 'none',
    lineHeight: '1.5',
  },
  textareaOutput: {
    flex: 1,
    backgroundColor: '#020617',
    color: '#4ade80',
    border: 'none',
    padding: '1rem',
    fontFamily: '"Fira Code", "Courier New", monospace',
    fontSize: '0.95rem',
    resize: 'none',
    outline: 'none',
    lineHeight: '1.5',
  },
  footer: {
    textAlign: 'center',
    padding: '1rem',
    backgroundColor: '#1e293b',
    borderTop: '1px solid #334155',
    fontSize: '0.8rem',
    color: '#64748b',
  },
};
