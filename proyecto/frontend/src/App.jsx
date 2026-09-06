import { useState } from "react";
import "./App.css";

const API_URL = "http://localhost:8080/analizar";

function App() {
  const [comandos, setComandos] = useState("");
  const [resultados, setResultados] = useState([]);
  const [cargando, setCargando] = useState(false);

  const analizar = async () => {
    const lineas = comandos
      .split("\n")
      .map((l) => l.trim())
      .filter((l) => l.length > 0 && !l.startsWith("#"));

    if (lineas.length === 0) return;

    setCargando(true);
    const nuevosResultados = [];

    for (const linea of lineas) {
      try {
        const res = await fetch(API_URL, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ comando: linea }),
        });
        const data = await res.json();
        nuevosResultados.push({ linea, ...data });
      } catch (error) {
        nuevosResultados.push({
          linea,
          comando: "",
          valido: false,
          errores: ["No se pudo conectar con el servidor: " + error.message],
        });
      }
    }

    setResultados(nuevosResultados);
    setCargando(false);
  };

  return (
    <div className="contenedor">
      <h1>Analizador Léxico y Sintáctico - Comandos EXT2</h1>

      <textarea
        className="entrada"
        placeholder="Escribe uno o varios comandos, uno por línea..."
        value={comandos}
        onChange={(e) => setComandos(e.target.value)}
        rows={10}
      />

      <button onClick={analizar} disabled={cargando}>
        {cargando ? "Analizando..." : "Analizar"}
      </button>

      <div className="consola">
        {resultados.length === 0 && (
          <p className="vacio">Los resultados aparecerán aquí...</p>
        )}
        {resultados.map((r, i) => (
          <div key={i} className={`linea-resultado ${r.valido ? "valido" : "invalido"}`}>
            <div className="comando-texto">$ {r.linea}</div>
            {r.valido ? (
              <div className="ok">Comando válido</div>
            ) : (
              <ul className="errores">
                {r.errores.map((e, j) => (
                  <li key={j}>{e}</li>
                ))}
              </ul>
            )}
          </div>
        ))}
      </div>
    </div>
  );
}

export default App;