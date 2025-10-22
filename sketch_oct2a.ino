#include <WiFi.h> // Librería para manejar la conexión WiFi del ESP32.
#include <WebServer.h>  // Librería para crear un servidor web en el ESP32.
// Pines para los LEDs del semáforo y el buzzer.
#define PIN_ROJO     27
#define PIN_AMARILLO 26
#define PIN_VERDE    32
#define PIN_BUZZER   25
const char* ssid = "WIFI_UCC_ESTUDIANTES"; // Nombre de la red wifi.
const char* password = "E5tud14nt3s_BplC00r*"; // Contraseña de la red WiFi a conectar.
WebServer server(80); // Crea un servidor web en el puerto 80.

// Variables globales para el temporizador y control de estados.
unsigned long tiempoInicioEstado = 0;  // Tiempo de inicio del estado actual en milisegundos.
unsigned long duracionEstado = 0;      // Duración del estado en milisegundos.
bool estadoConTemporizador = false;     // Indica si el estado actual tiene temporizador activo.
bool sonidoActivado = true;             // Indica si el sonido del buzzer está activado.
String descripcionEstado = "";          // Descripción del estado actual.
bool primerPitidoVerde = true;         // Indica si es el primer pitido para el estado verde.
bool primerPitidoAmarillo = true;      // Indica si es el primer pitido para el estado amarillo.
bool primerPitidoRojo = true;          // Indica si es el primer pitido para el estado rojo.
bool turnoTerminado = false;            // Indica si el turno ha terminado.
unsigned long tiempoInicioTurnoTerminado = 0;  // Tiempo de inicio del turno terminado en milisegundos.
// Variables para el historial de estados.
String historialEstados = "";          // Almacena el historial de estados como texto.
String ultimoMensajeHistorial = "";     // Almacena el último mensaje del historial.
String idEstadoActivo = "";             // Identificador del estado activo.

// Prototipos de funciones para el servidor.
void manejarRaiz(); // Maneja la página principal.
void setEstado(); // Establece el estado del semáforo.
void terminarTurno(); // Termina el turno actual.
void apagarEstado(); // Apaga el estado actual.
void limpiarHistorial(); // Limpia el historial de estados.
void obtenerEstado(); // Obtiene el estado actual.
void obtenerTiempoRestante(); // Obtiene el tiempo restante del temporizador.

void setup() {
  Serial.begin(115200); // Inicia la comunicación serial a 115200 baudios.
  // Configuración de los pines de los LEDs y el buzzer como salidas.
  pinMode(PIN_ROJO, OUTPUT);
  pinMode(PIN_AMARILLO, OUTPUT);
  pinMode(PIN_VERDE, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  // Iniciar con todos los LEDs apagados.
  digitalWrite(PIN_ROJO, LOW);
  digitalWrite(PIN_AMARILLO, LOW);
  digitalWrite(PIN_VERDE, LOW);
  noTone(PIN_BUZZER); // Asegura que el buzzer esté apagado al inicio.
  // Conectar a WiFi al encender el ESP32.
  conectarWiFi();
  // Iniciar el servidor web.
  iniciarServidorWeb();
}

void loop() {
  server.handleClient(); // Maneja las solicitudes HTTP entrantes del cliente.

  // Lógica para el turno terminado: mantiene todos los LEDs encendidos y emite un pitido cada segundo por 1 minuto.
  if (turnoTerminado) {
    if (millis() - tiempoInicioTurnoTerminado <= 60000) { // Mantener por 1 minuto.
      digitalWrite(PIN_ROJO, HIGH);
      digitalWrite(PIN_AMARILLO, HIGH);
      digitalWrite(PIN_VERDE, HIGH);
      static unsigned long tiempoAnteriorPitido = millis();
      if (millis() - tiempoAnteriorPitido >= 1000) { // Cada segundo.
        tiempoAnteriorPitido = millis();
        tone(PIN_BUZZER, 100, 500); // Emite un pitido de 100Hz por 500ms.
      }
    } else {
      turnoTerminado = false;
      digitalWrite(PIN_ROJO, LOW);
      digitalWrite(PIN_AMARILLO, LOW);
      digitalWrite(PIN_VERDE, LOW);
      noTone(PIN_BUZZER);
      ultimoMensajeHistorial = "Página recargada: turno terminado";
    }
  }

  // Lógica para el temporizador de estado: apaga el estado si el temporizador se cumple.
  if (estadoConTemporizador && !turnoTerminado) {
    if (millis() - tiempoInicioEstado >= duracionEstado) {
      estadoConTemporizador = false;
      digitalWrite(PIN_ROJO, LOW);
      digitalWrite(PIN_AMARILLO, LOW);
      digitalWrite(PIN_VERDE, LOW);
      noTone(PIN_BUZZER);
      ultimoMensajeHistorial = "Estado apagado: temporizador cumplido";
      idEstadoActivo = "";
    }
  }

  // Lógica para los pitidos según el estado (sin temporizador).
  if (sonidoActivado && !turnoTerminado && !estadoConTemporizador) {
    // Pitidos para el estado verde: 3 pitidos cada 5 minutos.
    if (digitalRead(PIN_VERDE) == HIGH) {
      if (primerPitidoVerde) {
        for (int i = 0; i < 3; i++) {
          tone(PIN_BUZZER, 100, 1000);
          delay(1500);
        }
        primerPitidoVerde = false;
      } else {
        static unsigned long tiempoAnteriorVerde = millis();
        if (millis() - tiempoAnteriorVerde >= 300000) { // Cada 5 minutos.
          tiempoAnteriorVerde = millis();
          for (int i = 0; i < 3; i++) {
            tone(PIN_BUZZER, 100, 1000);
            delay(1500);
          }
        }
      }
    } else {
      primerPitidoVerde = true; // Reinicia el flag si el LED verde se apaga.
    }

    // Pitidos para el estado amarillo: un pitido largo cada 10 minutos.
    if (digitalRead(PIN_AMARILLO) == HIGH) {
      if (primerPitidoAmarillo) {
        tone(PIN_BUZZER, 100, 3000); // Pitido largo de 3 segundos.
        delay(4000);
        primerPitidoAmarillo = false;
      } else {
        static unsigned long tiempoAnteriorAmarillo = millis();
        if (millis() - tiempoAnteriorAmarillo >= 600000) { // Cada 10 minutos.
          tiempoAnteriorAmarillo = millis();
          tone(PIN_BUZZER, 100, 3000);
          delay(4000);
        }
      }
    } else {
      primerPitidoAmarillo = true; // Reinicia el flag si el LED amarillo se apaga.
    }

    // Pitidos para el estado rojo: 6 pitidos cada 2 minutos.
    if (digitalRead(PIN_ROJO) == HIGH) {
      if (primerPitidoRojo) {
        for (int i = 0; i < 6; i++) {
          tone(PIN_BUZZER, 100, 1000);
          delay(1500);
        }
        primerPitidoRojo = false;
      } else {
        static unsigned long tiempoAnteriorRojo = millis();
        if (millis() - tiempoAnteriorRojo >= 120000) { // Cada 2 minutos.
          tiempoAnteriorRojo = millis();
          for (int i = 0; i < 6; i++) {
            tone(PIN_BUZZER, 100, 1000);
            delay(1500);
          }
        }
      }
    } else {
      primerPitidoRojo = true; // Reinicia el flag si el LED rojo se apaga.
    }
  }

  // Lógica para los pitidos según el estado CON TEMPORIZADOR.
  if (sonidoActivado && !turnoTerminado && estadoConTemporizador) {
    // Pitidos para el estado verde con temporizador.
    if (digitalRead(PIN_VERDE) == HIGH) {
      if (primerPitidoVerde) {
        for (int i = 0; i < 3; i++) {
          if (millis() - tiempoInicioEstado < duracionEstado) {
            tone(PIN_BUZZER, 100, 1000);
            delay(1500);
          } else {
            break; // Sale del bucle si el temporizador se cumplió.
          }
        }
        primerPitidoVerde = false;
      } else {
        static unsigned long tiempoAnteriorVerde = millis();
        if (millis() - tiempoAnteriorVerde >= 300000) { // Cada 5 minutos.
          tiempoAnteriorVerde = millis();
          for (int i = 0; i < 3; i++) {
            if (millis() - tiempoInicioEstado < duracionEstado) {
              tone(PIN_BUZZER, 100, 1000);
              delay(1500);
            } else {
              break; // Sale del bucle si el temporizador se cumplió.
            }
          }
        }
      }
    }

    // Pitidos para el estado amarillo con temporizador.
    if (digitalRead(PIN_AMARILLO) == HIGH) {
      if (primerPitidoAmarillo) {
        if (millis() - tiempoInicioEstado < duracionEstado) {
          tone(PIN_BUZZER, 100, 3000);
          delay(4000);
        }
        primerPitidoAmarillo = false;
      } else {
        static unsigned long tiempoAnteriorAmarillo = millis();
        if (millis() - tiempoAnteriorAmarillo >= 600000) { // Cada 10 minutos.
          tiempoAnteriorAmarillo = millis();
          if (millis() - tiempoInicioEstado < duracionEstado) {
            tone(PIN_BUZZER, 100, 3000);
            delay(4000);
          }
        }
      }
    }

    // Pitidos para el estado rojo con temporizador.
    if (digitalRead(PIN_ROJO) == HIGH) {
      if (primerPitidoRojo) {
        for (int i = 0; i < 6; i++) {
          if (millis() - tiempoInicioEstado < duracionEstado) {
            tone(PIN_BUZZER, 100, 1000);
            delay(1500);
          } else {
            break; // Sale del bucle si el temporizador se cumplió.
          }
        }
        primerPitidoRojo = false;
      } else {
        static unsigned long tiempoAnteriorRojo = millis();
        if (millis() - tiempoAnteriorRojo >= 120000) { // Cada 2 minutos.
          tiempoAnteriorRojo = millis();
          for (int i = 0; i < 6; i++) {
            if (millis() - tiempoInicioEstado < duracionEstado) {
              tone(PIN_BUZZER, 100, 1000);
              delay(1500);
            } else {
              break; // Sale del bucle si el temporizador se cumplió.
            }
          }
        }
      }
    }
  }
}

// Función para conectar a la red WiFi.
void conectarWiFi() {
  WiFi.begin(ssid, password); // Inicia la conexión a la red WiFi.
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) { // Espera hasta que se conecte.
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Conectado a WiFi. IP: ");
  Serial.println(WiFi.localIP()); // Muestra la IP asignada al ESP32.
}

// Función para iniciar el servidor web y definir las rutas.
void iniciarServidorWeb() {
  // Ruta principal
  server.on("/", manejarRaiz);
  // Rutas para establecer el estado.
  server.on("/setEstado", HTTP_GET, setEstado);
  // Ruta para terminar turno
  server.on("/terminarTurno", HTTP_GET, terminarTurno);
  // Ruta para obtener el estado actual.
  server.on("/obtenerEstado", HTTP_GET, obtenerEstado);
  // Ruta para obtener el tiempo restante.
  server.on("/obtenerTiempoRestante", HTTP_GET, obtenerTiempoRestante);
  // Ruta para apagar los estados.
  server.on("/apagarEstado", HTTP_GET, apagarEstado);
  // Ruta para limpiar historial.
  server.on("/limpiarHistorial", HTTP_GET, limpiarHistorial);
  // Iniciar servidor.
  server.begin();
  Serial.println("Servidor web iniciado");
}

// Función para obtener el estado actual y enviarlo al cliente.
void obtenerEstado() {
  String mensaje = ultimoMensajeHistorial;
  ultimoMensajeHistorial = "";
  server.send(200, "text/plain", mensaje);
}

// Función para obtener el tiempo restante del temporizador y enviarlo al cliente.
void obtenerTiempoRestante() {
  if (estadoConTemporizador) {
    unsigned long tiempoRestante = (duracionEstado - (millis() - tiempoInicioEstado)) / 1000;
    if (tiempoRestante > 0) {
      int horas = tiempoRestante / 3600;
      int minutos = (tiempoRestante % 3600) / 60;
      int segundos = tiempoRestante % 60;
      String tiempoStr = String(horas) + ":" + String(minutos) + ":" + String(segundos);
      server.send(200, "text/plain", tiempoStr);
    } else {
      server.send(200, "text/plain", "0:0:0");
    }
  } else {
    server.send(200, "text/plain", "0:0:0");
  }
}

// Función para manejar la página principal del servidor.
void manejarRaiz() {
  String html = R"=====(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>Estado del Trabajador</title>
    <style>
        /* Estilos CSS para la interfaz */
        body {
            font-family: 'Comic Sans MS', cursive, sans-serif;
            background-color: #f0f8ff;
            margin: 0;
            padding: 20px;
            text-align: center;
            position: relative;
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
        }
        .main-container {
            width: 100%;
            max-width: 500px;
            position: relative;
        }
        h1 {
            color: #2c3e50;
            margin-bottom: 30px;
            font-size: 24px;
        }
        .container {
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 15px;
            width: 100%;
            margin: 0 auto;
            background-color: rgba(255, 255, 255, 0.8);
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
            box-sizing: border-box;
        }
        .button {
            padding: 12px 20px;
            font-size: 16px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            transition: all 0.3s;
            width: 100%;
        }
        .button.on {
            opacity: 1;
        }
        .button.off {
            opacity: 0.7;
        }
        .red {
            background-color: #e74c3c;
            color: white;
        }
        .yellow {
            background-color: #f39c12;
            color: white;
        }
        .green {
            background-color: #2ecc71;
            color: white;
        }
        .end-turn {
            background-color: #8e44ad;
            color: white;
        }
        .cat-message {
            margin-top: 20px;
            font-style: italic;
            color: #7f8c8d;
            font-size: 14px;
        }
        .button:hover {
            transform: scale(1.05);
        }
        textarea {
            width: 100%;
            padding: 10px;
            border-radius: 5px;
            border: 1px solid #ccc;
            margin-bottom: 10px;
            resize: none;
            font-size: 14px;
        }
        .checkbox-container {
            display: flex;
            align-items: center;
            gap: 10px;
            margin-bottom: 10px;
        }
        .historial {
            margin-top: 20px;
            text-align: left;
            width: 100%;
            max-height: 150px;
            overflow-y: auto;
            border: 1px solid #ccc;
            padding: 10px;
            border-radius: 5px;
            background-color: white;
            font-size: 14px;
        }
        .historial-container {
            display: flex;
            justify-content: space-between;
            align-items: center;
            width: 100%;
        }
        #catImage {
            width: 100px;
            height: auto;
            margin: 10px auto;
            display: block;
        }
        .dialog {
            display: none;
            position: fixed;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            background-color: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            z-index: 1000;
            width: 80%;
            max-width: 400px;
        }
        .dialog-buttons {
            display: flex;
            gap: 10px;
            margin-top: 15px;
        }
        .dialog-button {
            padding: 10px 20px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 14px;
        }
        .dialog-confirm {
            background-color: #8e44ad;
            color: white;
        }
        .dialog-cancel {
            background-color: #95a5a6;
            color: white;
        }
        .clear-historial-button {
            padding: 10px 20px;
            font-size: 14px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            transition: all 0.3s;
            background-color: #95a5a6;
            color: white;
            margin-top: 10px;
        }
        @media (max-width: 600px) {
            h1 {
                font-size: 20px;
            }
            .container {
                padding: 15px;
            }
            .button {
                padding: 10px 15px;
                font-size: 14px;
            }
            textarea {
                font-size: 12px;
            }
        }
    </style>
</head>
<body>
    <!-- Contenedor principal de la interfaz -->
    <div class="main-container">
        <div class="container">
            <h1>Marca tu Estado, Trabajador</h1>
            <form id="estadoForm">
                <!-- Campo para describir el estado -->
                <textarea id="descripcion" name="descripcion" placeholder="Describe tu estado (obligatorio)" required></textarea>
                <!-- Checkbox para activar/desactivar el sonido -->
                <div class="checkbox-container">
                    <input type="checkbox" id="sonido" name="sonido" checked>
                    <label for="sonido">Activar sonido</label>
                </div>
                <!-- Botones para cambiar el estado -->
                <div style="display: flex; gap: 10px; width: 100%;">
                    <button class="button red" type="button" onclick="showTemporizadorDialog('rojo')">🚨 Emergencia</button>
                </div>
                <div style="display: flex; gap: 10px; width: 100%;">
                    <button class="button yellow" type="button" onclick="showTemporizadorDialog('amarillo')">🔧 Ocupado</button>
                </div>
                <div style="display: flex; gap: 10px; width: 100%;">
                    <button class="button green" type="button" onclick="showTemporizadorDialog('verde')">🟢 Disponible</button>
                </div>
                <div style="display: flex; gap: 10px; width: 100%;">
                    <button class="button end-turn" type="button" onclick="showTerminarTurnoDialog()">🛑 Terminar Turno</button>
                </div>
                <div style="display: flex; gap: 10px; margin-top: 10px; width: 100%;">
                    <button class="button" type="button" onclick="showApagarEstadoDialog()" style="background-color: #95a5a6; color: white;">🛑 Apagar Estado Actual</button>
                </div>
            </form>
            <!-- Imagen del gato motivacional -->
            <img id="catImage" src="https://cataas.com/cat?type=small" alt="Gato Motivador">
            <!-- Historial de estados -->
            <div class="historial">
                <div class="historial-container">
                    <h3 style="margin: 0;">Historial de Estados:</h3>
                    <button class="clear-historial-button" onclick="showLimpiarHistorialDialog()">Limpiar Historial</button>
                </div>
                <div id="historialEstados"></div>
            </div>
            <!-- Mensaje motivacional -->
            <p class="cat-message">
                ¡Hola, buen trabajador! 🐱<br>
                No olvides marcar tu estado para que todos sepan cómo estás, en especial tu querido jefe.
            </p>
        </div>
    </div>

    <!-- Diálogo para terminar el turno -->
    <div id="terminarTurnoDialog" class="dialog">
        <h3>¿Por qué terminas el turno?</h3>
        <textarea id="descripcionTerminarTurno" placeholder="Describe la razón (obligatorio)" required style="width: 100%; padding: 10px; border-radius: 5px; border: 1px solid #ccc; margin-bottom: 10px; resize: none;"></textarea>
        <div class="dialog-buttons">
            <button class="dialog-button dialog-confirm" onclick="terminarTurnoConDescripcion()">Confirmar</button>
            <button class="dialog-button dialog-cancel" onclick="document.getElementById('terminarTurnoDialog').style.display = 'none'">Cancelar</button>
        </div>
    </div>

    <!-- Diálogo para configurar el temporizador -->
    <div id="temporizadorDialog" class="dialog">
        <h3>Configurar Temporizador</h3>
        <p>¿Quieres activar un temporizador para este estado?</p>
        <div style="display: flex; align-items: center; gap: 10px; margin-bottom: 10px;">
            <input type="checkbox" id="activarTemporizador" name="activarTemporizador">
            <label for="activarTemporizador">Activar temporizador</label>
        </div>
        <div id="temporizadorOpciones" style="display: none; margin-bottom: 10px;">
            <div style="display: flex; gap: 10px; align-items: center;">
                <input type="number" id="horas" name="horas" min="0" max="23" placeholder="0" style="width: 60px; padding: 5px;">
                <span>Horas</span>
            </div>
            <div style="display: flex; gap: 10px; align-items: center;">
                <input type="number" id="minutos" name="minutos" min="0" max="59" placeholder="0" style="width: 60px; padding: 5px;">
                <span>Minutos</span>
            </div>
            <div style="display: flex; gap: 10px; align-items: center;">
                <input type="number" id="segundos" name="segundos" min="0" max="59" placeholder="0" style="width: 60px; padding: 5px;">
                <span>Segundos</span>
            </div>
        </div>
        <div class="dialog-buttons">
            <button class="dialog-button dialog-confirm" onclick="confirmarTemporizador()">Confirmar</button>
            <button class="dialog-button dialog-cancel" onclick="document.getElementById('temporizadorDialog').style.display = 'none'">Cancelar</button>
        </div>
    </div>

    <!-- Diálogo para apagar el estado actual -->
    <div id="apagarEstadoDialog" class="dialog">
        <h3>¿Estás seguro que quieres apagar el estado actual?</h3>
        <div class="dialog-buttons">
            <button class="dialog-button dialog-confirm" onclick="apagarEstadoConfirmado()">Confirmar</button>
            <button class="dialog-button dialog-cancel" onclick="document.getElementById('apagarEstadoDialog').style.display = 'none'">Cancelar</button>
        </div>
    </div>

    <!-- Diálogo para limpiar el historial -->
    <div id="limpiarHistorialDialog" class="dialog">
        <h3>¿Estás seguro que quieres limpiar el historial?</h3>
        <div class="dialog-buttons">
            <button class="dialog-button dialog-confirm" onclick="limpiarHistorialConfirmado()">Confirmar</button>
            <button class="dialog-button dialog-cancel" onclick="document.getElementById('limpiarHistorialDialog').style.display = 'none'">Cancelar</button>
        </div>
    </div>

    <!-- Diálogo para mostrar mensajes -->
    <div id="mensajeDialog" class="dialog">
        <h3 id="mensajeDialogTitulo">Mensaje</h3>
        <p id="mensajeDialogTexto">Contenido del mensaje.</p>
        <div class="dialog-buttons">
            <button class="dialog-button dialog-confirm" onclick="document.getElementById('mensajeDialog').style.display = 'none'">Aceptar</button>
        </div>
    </div>

    <script>
        // Variables globales para el manejo de estados y temporizadores
        let historial = [];
        let estadoActual = null;
        let activarTemporizadorFlag = false;
        let horasTemporizador = 0;
        let minutosTemporizador = 0;
        let segundosTemporizador = 0;
        let tiempoRestanteInterval;
        let idEstadoActivo = null;

        // Actualiza la imagen del gato motivacional.
        function updateCatImage() {
            const catImage = document.getElementById('catImage');
            catImage.src = `https://cataas.com/cat?type=small&timestamp=${new Date().getTime()}`;
        }

        // Muestra el diálogo para configurar el temporizador.
        function showTemporizadorDialog(estado) {
            estadoActual = estado;
            document.getElementById('temporizadorDialog').style.display = 'block';
        }

        // Confirma la configuración del temporizador y envía la solicitud al servidor.
        function confirmarTemporizador() {
            const activarTemporizador = document.getElementById('activarTemporizador').checked;
            if (activarTemporizador) {
                horasTemporizador = parseInt(document.getElementById('horas').value) || 0;
                minutosTemporizador = parseInt(document.getElementById('minutos').value) || 0;
                segundosTemporizador = parseInt(document.getElementById('segundos').value) || 0;

                // Validar que al menos uno de los campos de tiempo sea mayor a 0
                if (horasTemporizador === 0 && minutosTemporizador === 0 && segundosTemporizador === 0) {
                    mostrarMensajeDialog("Debes marcar un tiempo, no dejarlo en 0.");
                    return;
                }

                activarTemporizadorFlag = true;
            } else {
                activarTemporizadorFlag = false;
            }
            document.getElementById('temporizadorDialog').style.display = 'none';
            setEstadoConTemporizador(estadoActual);
        }

        // Envía la solicitud al servidor para establecer el estado con temporizador.
        function setEstadoConTemporizador(estado) {
            const descripcion = document.getElementById('descripcion').value;
            const sonido = document.getElementById('sonido').checked;
            if (!descripcion) {
                mostrarMensajeDialog("Por favor, ingresa una descripción de tu estado.");
                return;
            }
            let url = `/setEstado?estado=${estado}&descripcion=${encodeURIComponent(descripcion)}&sonido=${sonido}`;
            if (activarTemporizadorFlag) {
                const totalSegundos = horasTemporizador * 3600 + minutosTemporizador * 60 + segundosTemporizador;
                url += `&temporizador=true&duracion=${totalSegundos}`;
            }
            const xhr = new XMLHttpRequest();
            xhr.open('GET', url, true);
            xhr.onload = function() {
                if (xhr.status === 200) {
                    const respuesta = xhr.responseText;
                    idEstadoActivo = Date.now();
                    actualizarHistorial(respuesta, idEstadoActivo);
                    document.getElementById('descripcion').value = '';
                    updateCatImage();
                    if (activarTemporizadorFlag) {
                        clearInterval(tiempoRestanteInterval);
                        tiempoRestanteInterval = setInterval(actualizarTiempoRestante, 1000);
                    }
                }
            };
            xhr.send();
        }

        // Muestra el diálogo para terminar el turno.
        function showTerminarTurnoDialog() {
            document.getElementById('terminarTurnoDialog').style.display = 'block';
        }

        // Envía la solicitud al servidor para terminar el turno.
        function terminarTurnoConDescripcion() {
            const descripcion = document.getElementById('descripcionTerminarTurno').value;
            if (!descripcion) {
                mostrarMensajeDialog("Por favor, ingresa una descripción para terminar el turno.");
                return;
            }
            mostrarMensajeDialog("La página se recargará automáticamente en un minuto.");
            const xhr = new XMLHttpRequest();
            xhr.open('GET', `/terminarTurno?descripcion=${encodeURIComponent(descripcion)}`, true);
            xhr.onload = function() {
                if (xhr.status === 200) {
                    const respuesta = xhr.responseText;
                    actualizarHistorial(respuesta);
                    document.getElementById('terminarTurnoDialog').style.display = 'none';
                    updateCatImage();
                }
            };
            xhr.send();
        }

        // Muestra el diálogo para apagar el estado actual.
        function showApagarEstadoDialog() {
            document.getElementById('apagarEstadoDialog').style.display = 'block';
        }

        // Envía la solicitud al servidor para apagar el estado actual.
        function apagarEstadoConfirmado() {
            const xhr = new XMLHttpRequest();
            xhr.open('GET', '/apagarEstado', true);
            xhr.onload = function() {
                if (xhr.status === 200) {
                    const respuesta = xhr.responseText;
                    actualizarHistorial(respuesta);
                    updateCatImage();
                    clearInterval(tiempoRestanteInterval);
                    idEstadoActivo = null;
                    document.getElementById('apagarEstadoDialog').style.display = 'none';
                }
            };
            xhr.send();
        }

        // Muestra el diálogo para limpiar el historial.
        function showLimpiarHistorialDialog() {
            document.getElementById('limpiarHistorialDialog').style.display = 'block';
        }

        // Envía la solicitud al servidor para limpiar el historial.
        function limpiarHistorialConfirmado() {
            const xhr = new XMLHttpRequest();
            xhr.open('GET', '/limpiarHistorial', true);
            xhr.onload = function() {
                if (xhr.status === 200) {
                    historial = [];
                    document.getElementById('historialEstados').innerHTML = '';
                    updateCatImage();
                    clearInterval(tiempoRestanteInterval);
                    idEstadoActivo = null;
                    document.getElementById('limpiarHistorialDialog').style.display = 'none';
                }
            };
            xhr.send();
        }

        // Limpia el historial de estados.
        function limpiarHistorial() {
            historial = [];
            document.getElementById('historialEstados').innerHTML = '';
            updateCatImage();
            clearInterval(tiempoRestanteInterval);
            idEstadoActivo = null;
        }

        // Muestra un mensaje en el diálogo de mensaje.
        function mostrarMensajeDialog(mensaje) {
            document.getElementById('mensajeDialogTexto').textContent = mensaje;
            document.getElementById('mensajeDialog').style.display = 'block';
        }

        // Actualiza el historial de estados en la interfaz.
        function actualizarHistorial(respuesta, id = null) {
            const historialDiv = document.getElementById('historialEstados');
            const ahora = new Date();
            const horaExacta = ahora.toLocaleTimeString();
            const fechaExacta = ahora.toLocaleDateString();
            const mensajeConHora = `[${fechaExacta} ${horaExacta}] ${respuesta}`;
            historial.unshift({ id: id || Date.now(), texto: mensajeConHora });
            if (historial.length > 10) {
                historial.pop();
            }
            historialDiv.innerHTML = historial.map(entry => entry.texto).join('<br>');
        }

        // Actualiza el tiempo restante del temporizador en la interfaz.
        function actualizarTiempoRestante() {
            const xhr = new XMLHttpRequest();
            xhr.open('GET', '/obtenerTiempoRestante', true);
            xhr.onload = function() {
                if (xhr.status === 200) {
                    const tiempoRestante = xhr.responseText;
                    if (tiempoRestante === "0:0:0") {
                        clearInterval(tiempoRestanteInterval);
                        verificarEstado();
                    } else {
                        const entries = historial.map(entry => {
                            if (entry.id === idEstadoActivo && entry.texto.includes('Temporizador:')) {
                                return entry.texto.replace(/Tiempo restante: \d+:\d+:\d+/, `Tiempo restante: ${tiempoRestante}`);
                            }
                            return entry.texto.replace(/ \| Tiempo restante: \d+:\d+:\d+/, '');
                        });
                        document.getElementById('historialEstados').innerHTML = entries.join('<br>');
                    }
                }
            };
            xhr.send();
        }

        // Verifica el estado actual del servidor.
        function verificarEstado() {
            const xhr = new XMLHttpRequest();
            xhr.open('GET', '/obtenerEstado', true);
            xhr.onload = function() {
                if (xhr.status === 200) {
                    const mensaje = xhr.responseText;
                    if (mensaje) {
                        if (mensaje.includes("Página recargada")) {
                            window.location.reload();
                        } else if (mensaje.includes("temporizador cumplido")) {
                            const updatedEntries = historial.map(entry => {
                                if (entry.id === idEstadoActivo && entry.texto.includes('Temporizador:')) {
                                    return entry.texto.replace(/ \| Tiempo restante: \d+:\d+:\d+/, '');
                                }
                                return entry.texto;
                            });
                            document.getElementById('historialEstados').innerHTML = updatedEntries.join('<br>');
                            actualizarHistorial(mensaje);
                            idEstadoActivo = null;
                        }
                    }
                }
            };
            xhr.send();
        }

        // Muestra u oculta las opciones del temporizador según el checkbox.
        document.getElementById('activarTemporizador').addEventListener('change', function() {
            const opciones = document.getElementById('temporizadorOpciones');
            opciones.style.display = this.checked ? 'block' : 'none';
        });

        // Verifica el estado del servidor cada segundo.
        setInterval(verificarEstado, 1000);
    </script>
</body>
</html>
  )=====";
  server.send(200, "text/html", html);
}

// Función para establecer el estado del semáforo según los parámetros recibidos.
void setEstado() {
  if (server.hasArg("descripcion") && server.hasArg("estado")) {
    descripcionEstado = server.arg("descripcion");
    sonidoActivado = server.arg("sonido") == "true";
    if (server.hasArg("temporizador") && server.arg("temporizador") == "true") {
      estadoConTemporizador = true;
      duracionEstado = server.arg("duracion").toInt() * 1000; // Convertir a milisegundos
      tiempoInicioEstado = millis();
    } else {
      estadoConTemporizador = false;
    }
    if (server.arg("estado") == "rojo") {
      digitalWrite(PIN_ROJO, HIGH);
      digitalWrite(PIN_AMARILLO, LOW);
      digitalWrite(PIN_VERDE, LOW);
      primerPitidoRojo = true;
    } else if (server.arg("estado") == "amarillo") {
      digitalWrite(PIN_AMARILLO, HIGH);
      digitalWrite(PIN_ROJO, LOW);
      digitalWrite(PIN_VERDE, LOW);
      primerPitidoAmarillo = true;
    } else if (server.arg("estado") == "verde") {
      digitalWrite(PIN_VERDE, HIGH);
      digitalWrite(PIN_ROJO, LOW);
      digitalWrite(PIN_AMARILLO, LOW);
      primerPitidoVerde = true;
    }
    String mensaje = "Estado: " + String(server.arg("estado")) + " | Descripción: " + descripcionEstado + " | Sonido: " + (sonidoActivado ? "Activado" : "Desactivado");
    if (estadoConTemporizador) {
      int horas = duracionEstado / (1000 * 3600);
      int minutos = (duracionEstado % (1000 * 3600)) / (1000 * 60);
      int segundos = (duracionEstado % (1000 * 60)) / 1000;
      mensaje += " | Temporizador: " + String(horas) + "h " + String(minutos) + "m " + String(segundos) + "s | Tiempo restante: " + String(horas) + ":" + String(minutos) + ":" + String(segundos);
    }
    server.send(200, "text/plain", mensaje);
  } else {
    server.send(400, "text/plain", "Faltan parámetros");
  }
}

// Función para terminar el turno y activar el modo de turno terminado.
void terminarTurno() {
  if (server.hasArg("descripcion")) {
    descripcionEstado = server.arg("descripcion");
    turnoTerminado = true;
    tiempoInicioTurnoTerminado = millis();
    String mensaje = "Turno Terminado | Descripción: " + descripcionEstado;
    server.send(200, "text/plain", mensaje);
  } else {
    server.send(400, "text/plain", "Falta descripción");
  }
}

// Función para limpiar el historial de estados.
void limpiarHistorial() {
  String mensaje = "Historial limpiado";
  server.send(200, "text/plain", mensaje);
}

// Función para apagar el estado actual y reiniciar las variables.
void apagarEstado() {
  turnoTerminado = false;
  estadoConTemporizador = false;
  digitalWrite(PIN_ROJO, LOW);
  digitalWrite(PIN_AMARILLO, LOW);
  digitalWrite(PIN_VERDE, LOW);
  noTone(PIN_BUZZER);
  String mensaje = "Estado: Apagado";
  server.send(200, "text/plain", mensaje);
}
