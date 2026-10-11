# Object detection H10 + Hiwonder

Variante nueva del programa
`object_detection_original_h10`. Conserva Hailo10H, cámara, Oradar, detección
YOLO y la lógica de misión, pero sustituye el REPL del LEGO SPIKE por la placa
Hiwonder mediante `hiwonder::Brick` y `spike_compat`.

El original no se modifica. Compila para dos robots:

| Robot | Acelerador | Lidar | Build | Arranque |
|---|---|---|---|---|
| H10 | Hailo-10H | Oradar MS200 | `build/h10_hiwonder` | `run_hiwonder.sh` |
| Hailo-8 | Hailo-8 HAT (26 TOPS) | RPLidar S2L | `build/h8_s2l` | `run_hiwonder_h8.sh` |

## Seguridad de arranque

Sin `HIWONDER_ARM=1`, el programa sólo inicia cámara, Hailo y LiDAR. No abre el
controlador Hiwonder ni crea el hilo de movimiento. En modo armado valida todos
los parámetros antes de abrir el puerto y, sin mover actuadores, comprueba:

- `MOTOR_EXT` versión 2 o posterior (firmware `pos-v8` recomendado: HOLD);
- control de posición en M4 y 1320 ticks por vuelta;
- telemetría IMU;
- calibración del bias del giroscopio (5 s, robot quieto).

Después arranca el pipeline Hailo y sólo entonces crea la misión. El movimiento
sigue esperando el botón físico GPIO4. El relé GPIO2 del SPIKE no se solicita
ni se pulsa.

## Compilar

`WRO_LIDAR` elige el lidar (`ORADAR` por defecto). Robot H10:

```bash
cmake -S . -B build/h10_hiwonder -DCMAKE_BUILD_TYPE=Release
make -C build/h10_hiwonder -j"$(nproc)"
```

Robot Hailo-8 con RPLidar S2L:

```bash
cmake -S . -B build/h8_s2l -DCMAKE_BUILD_TYPE=Release -DWRO_LIDAR=RPLIDAR_S2L
make -C build/h8_s2l -j"$(nproc)"
```

Después de configurar una vez, basta `make -C build/<nombre>`. El SDK Oradar
se reutiliza de sólo lectura desde `object_detection_original_h10/oradar_sdk`.
El de Slamtec se toma ya compilado de `~/rplidar_sdk` (otra ruta:
`-DRPLIDAR_SDK_DIR=...`).

El mismo HailoRT 4.23 sirve para los dos binarios, pero cada acelerador
necesita su HEF: `run_hiwonder_h8.sh` usa
`src/HailoModels/yolov8n_wro_hailo8.hef` (clases greenbox, redbox, xparking,
en el orden de `common_var.h`).

## Orientación del RPLidar S2L

El driver del S2L llena el buffer con la orientación del Oradar (derecha 0,
atrás 90, izquierda 180, frente 270), así que la misión no cambia. Supone el
S2L con su marca frontal hacia el frente del robot y boca arriba. Si se monta
de otra forma, se ajusta al configurar:

- `-DRPLIDAR_FRONT_DEG=N`: ángulo del S2L que mira al frente del robot.
- `-DRPLIDAR_CLOCKWISE=0`: si está boca abajo.

**Verificar antes de armar**, con el programa parado y un obstáculo a ~30 cm
delante del robot:

```bash
~/rplidar_sdk/output/Linux/Release/ultra_simple --channel --serial /dev/ttyUSB0 1000000
```

El ángulo (`theta`) que lee ~300 mm es `RPLIDAR_FRONT_DEG`. Mueve el obstáculo
a la derecha del robot: si `theta` aumenta, el sentido es horario
(`RPLIDAR_CLOCKWISE=1`).

## Probar percepción sin motores

```bash
./run_hiwonder.sh
```

Equivale al comando original con el mismo modelo y `--input rpi`, pero deja
`HIWONDER_ARM=0`.

## Armar, sólo después de calibrar

```bash
cp hiwonder.env.example hiwonder.env
# Editar y medir todos los campos vacíos.
./run_hiwonder.sh --arm
```

`--arm` habilita la conexión, no inicia el movimiento: todavía hay que pulsar
el botón físico. Si falta un valor, el firmware no tiene posición en M4, no llega IMU o
el puerto es ambiguo, el proceso termina antes de crear la misión.

## Diferencias inevitables respecto al SPIKE

- `Spike_Forward` y `Spike_Follow_Reference` ejecutan un paso de PD por llamada.
- Avance y giros conservan su semántica bloqueante y añaden timeout.
- `Spike_Hold_Motors` usa el HOLD de posición del firmware `pos-v8`; con
  firmware anterior, para el motor.
- `Spike_Advance_For_distance` usa diámetro real de rueda y un factor de
  distancia configurable, no la constante de la rueda LEGO.
- Los factores y ganancias del LEGO no se reutilizan automáticamente.

La placa debe tener `pos-v8` (control de posición en M4, límite 5.7 r/s, HOLD).

La librería de `hiwonder/` es la de `~/Jetson_cerebro/robotica/hiwonder_brick`
del 2026-10-04, más dos cosas propias de esta variante en `spike_compat`:
ganancias del PD configurables y `traction_direction`.

## Verificación realizada

En la Raspberry Pi 5 se configuró y compiló la variante completa con GCC 14.2.
Una ejecución desarmada de 10 segundos confirmó cámara RPi, Hailo10H y Oradar
simultáneos, sin abrir Hiwonder, con aproximadamente 29.95 FPS y salida limpia
al recibir `SIGINT`. Una ejecución con `HIWONDER_ARM=1`, sin placa ni
calibración, terminó antes de cargar el modelo y antes de crear el hilo de
misión, como exige la guarda de seguridad.
