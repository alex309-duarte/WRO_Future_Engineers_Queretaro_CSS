# Reto 2 (obstacle challenge) con la placa Hiwonder

La misión del reto 2 de `../object_detection_original_h10` (la del nacional,
con estacionamiento) corriendo sobre la placa Hiwonder en lugar del hub LEGO
SPIKE, con la **misma capa que el reto 1 ya probó en pista** (`src/ondevice`,
rama `reto1-hiwonder`). Cámara RPi, Hailo-8, RPLidar S2L, botón GPIO4.

| Robot | Acelerador | Lidar | Build | Arranque |
|---|---|---|---|---|
| Hailo-8 (este) | Hailo-8 HAT, HailoRT 4.23 | RPLidar S2L | `build/h8_s2l` | `./run_hiwonder_h8.sh` |
| Oradar MS200 | el que sea | Oradar MS200 | `cmake -DWRO_LIDAR=ORADAR` | `WRO_HIWONDER_BUILD_DIR=... ./run_hiwonder.sh` |

## Qué hay en esta carpeta

| Archivo | Qué es |
|---|---|
| `object_detection.cpp` | La misión original con el arranque/apagado de la placa Hiwonder (cambios marcados `Hiwonder:`) |
| `spike.h` | La API SPIKE de la misión, sobre `spike_compat` del reto 1 |
| `spike_reto2.cpp` | `Spike_Reto2_Init`, giros con `await`, avance en milímetros |
| `reto2_config.h` | Lo que el reto 1 no necesita: **diámetro de rueda**, escala de `tire_turn` |
| `lidar_reto2.cpp` | `Oradar_S2L_Advance_Until_Distance` con tipo de freno |
| `rasp_gpio.cpp` | Botón GPIO4 y LED GPIO3 (gpiochip0 de la Pi 5), sin relé |
| `tests/reto2_test.cpp` | 31 comprobaciones offline contra la placa falsa del reto 1 |

**No hay copia de la librería Hiwonder.** CMake compila directamente
`../../ondevice/hiwonder/`, `spike_hiwonder.cpp`, `parada_emergencia.cpp` y
`Oradar_S2L.cpp` del reto 1, y toda la calibración del robot (servo, PD de
rumbo, velocidad, parada de emergencia) se lee de
`../../ondevice/hiwonder_config.h`: lo que se ajuste ahí vale para los dos
retos. Ver `ORIGEN.md`.

## Compilar y probar sin mover nada

```bash
cmake -S . -B build/h8_s2l -DCMAKE_BUILD_TYPE=Release
make -C build/h8_s2l -j"$(nproc)"
./build/h8_s2l/reto2_test            # 31 comprobaciones, 0 fallos
(cd ../../ondevice && make test)     # las 51 del reto 1 siguen pasando
./run_hiwonder_h8.sh --solo-percepcion            # cámara + Hailo + lidar, la placa NO se abre
./run_hiwonder_h8.sh --solo-percepcion --no-display   # lo mismo por SSH, sin ventanas
```

`--solo-percepcion` abre las ventanas de la cámara (cajas con color y
confianza) y del lidar (paredes frente/derecha/izquierda/atrás detectadas). Es
la forma de comprobar el modelo y la orientación del lidar antes de armar.

## Correr el reto

```bash
WRO_MAX_RPS=1.5 ./run_hiwonder_h8.sh   # primeras pruebas, despacio
./run_hiwonder_h8.sh                   # velocidad de competencia (5.7 r/s)
./run_hiwonder_h8.sh --no-display      # por SSH / como servicio
```

Al arrancar: abre la placa, instala la parada de emergencia, comprueba que el
servo de dirección está dentro de su recorrido, lo **centra despacio** y
**calibra el giroscopio 5 s con el robot quieto**; luego levanta cámara,
Hailo y lidar, y la misión queda esperando el **botón** (el LED parpadea). El
sentido de la vuelta lo decide el lidar al salir del cajón (`distancia_derecha
> 600` = horario), como en el original.

**Ctrl+C detiene el robot en cualquier momento** (también `kill`, cerrar la
terminal o una caída): motores en COAST, servo sin par y fin del proceso. Es la
`parada_emergencia` del reto 1; no cubre `kill -9` ni un corte de energía de la
Pi (ahí: switch de motores o cortar los 12 V).

## Antes de la primera corrida: calibrar

Con las ruedas levantadas, en este orden. Las herramientas del reto 1 valen
(`~/Jetson_cerebro/robotica/hiwonder_brick/hiwonder-analisis-estatico/tools/`).

1. **Lo del reto 1** (signo del yaw, lado de la dirección): ya probado en
   pista; si se cambia `hiwonder_config.h`, cambia para los dos retos.
2. **Diámetro de rueda** (`kDiametroRuedaMm` en `reto2_config.h`, hoy 65 mm sin
   medir). Es lo único nuevo que hay que medir: con él
   `Spike_Advance_For_distance(…, mm, …)` recorre milímetros reales y los
   "grados del hub" del estacionamiento (`Spike_Advance_For_Degrees(-50, 2250, …)`)
   se convierten a la misma distancia que recorría el LEGO. Comprobación:
   `Spike_Advance_For_distance(50, 1000, 0)` debe recorrer 1000 mm; si recorre
   L mm, multiplicar el diámetro por L/1000.
3. **Giros con `tire_turn` parcial**. La misión gira con `tire_turn` 40 (0.96
   del tope), 45 (tope), 30 (0.72) y 15 (0.36): `Spike_Reto2_Init` escala el
   factor del reto 1 por los 4.29° por unidad de `turn()` del hub
   (`kGradosMotorPorTireTurn`). Los radios de giro cambian con el varillaje
   Ackermann, así que los ángulos de esquiva (`abs(angle_to_wall) ± 5`) y las
   distancias restadas a la hipotenusa (`- 400`, `- 250`…) se afinan en pista
   igual que se afinaron con el LEGO.
4. **Reversa** (estacionamiento). `Spike_Advance_For_Degrees` y los giros con
   `speed < 0` van hacia atrás; al reversear la corrección de rumbo invierte el
   signo y usa 0.15 del kp (como `ag_u()` del hub, sin término D):
   `reverse_kp_factor` / `reverse_kd_factor` en `spike_compat::Config`. Probar
   primero `Spike_Advance_For_Degrees(-50, 400, 0)` en una recta.

## Diferencias respecto al original (SPIKE)

- `Spike_Forward` y `Spike_Follow_Reference` ejecutan un paso de PD por
  llamada; los bucles del lidar los llaman cada 1 ms y el IMU llega a 20 Hz
  (el término D sólo ve cambio en 1 de cada ~50 llamadas). Si la dirección
  vibra en las rectas, es por esto.
- Avances por el lazo de posición del STM32 (más exactos que el `set_duty_cycle`
  del hub) y con timeout; `speed` es el tope de velocidad.
- `Spike_Hold_Motors` usa el HOLD de posición del firmware `pos-v8`;
  `Spike_Break_Motors` para la propulsión (el servo ya sostiene).
- `Advance_For_distance_Especial` (avance vigilado por el lidar): una lectura 0
  en el rayo vigilado ya no aborta el avance (el original la tomaba como
  "menos de 370 mm").
- Tres esperas que podían no salir nunca ahora tienen salida: la del cubo en
  `estacionamiento_*` (declaraba otra variable adentro: 3 s de espera y sigue),
  la de "pared válida" al calcular el ángulo de esquiva, y el cierre del
  barrido en la visualización (375° → 360°).
- Sin relé GPIO2 ni REPL: Ctrl+C antes de abrir la placa corta el pipeline
  limpiamente; después es parada de emergencia.

## Verificación realizada (2026-10-18, en el robot)

- Compila en la Pi 5 (GCC 14.2, HailoRT 4.23, OpenCV 4.10) sólo con los
  warnings que ya tenía el original.
- `reto2_test`: 31 comprobaciones, 0 fallos. `make test` del reto 1: 51, 0 fallos.
  El reto 1 (`my_lidar_app`) sigue compilando con la capa compartida.
- `--solo-percepcion --no-display` 20 s: RPLidar en `/dev/ttyUSB0` (frente
  1848 mm, derecha 629, izquierda 258), cámara 800x600, Hailo-8 a 29.97 FPS,
  salida limpia con Ctrl+C.
- Armado 30 s: servo leído en 854 (dentro de 625..875), centrado, giroscopio
  calibrado, `tire_turn 40 = 0.96`, lidar listo, pipeline arriba, esperando el
  botón; Ctrl+C → "Parada de emergencia: motores en COAST", proceso terminado y
  puerto de la placa libre. **Falta la prueba en pista con el botón.**
