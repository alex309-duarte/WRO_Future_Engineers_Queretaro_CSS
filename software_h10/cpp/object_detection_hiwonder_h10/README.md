# Object detection H10 + Hiwonder

Variante nueva del programa
`object_detection_original_h10`. Conserva Hailo10H, cámara, Oradar, detección
YOLO y la lógica de misión, pero sustituye el REPL del LEGO SPIKE por la placa
Hiwonder mediante `hiwonder::Brick` y `spike_compat`.

El original no se modifica. En la Raspberry esta variante vive en:

```text
/home/maker/WRO_Future_Engineers_Queretaro_CSS/software_h10/cpp/object_detection_hiwonder_h10
```

## Seguridad de arranque

Sin `HIWONDER_ARM=1`, el programa sólo inicia cámara, Hailo y LiDAR. No abre el
controlador Hiwonder ni crea el hilo de movimiento. En modo armado valida todos
los parámetros antes de abrir el puerto y, sin mover actuadores, comprueba:

- `MOTOR_EXT` versión 1;
- M1 habilitado y 1320 ticks por vuelta;
- telemetría IMU;
- calibración del bias del giroscopio.

Después arranca el pipeline Hailo y sólo entonces crea la misión. El movimiento
sigue esperando el botón físico GPIO4. El relé GPIO2 del SPIKE no se solicita
ni se pulsa.

## Compilar

```bash
cd ~/WRO_Future_Engineers_Queretaro_CSS/software_h10/cpp/object_detection_hiwonder_h10
cmake -S . -B build/h10_hiwonder -DCMAKE_BUILD_TYPE=Release
cmake --build build/h10_hiwonder -j"$(nproc)"
```

El SDK Oradar se reutiliza de sólo lectura desde el directorio hermano
`object_detection_original_h10/oradar_sdk`.

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
el botón físico. Si falta un valor, el firmware no es `pos-v4`, no llega IMU o
el puerto es ambiguo, el proceso termina antes de crear la misión.

## Diferencias inevitables respecto al SPIKE

- `Spike_Forward` y `Spike_Follow_Reference` ejecutan un paso de PD por llamada.
- Avance y giros conservan su semántica bloqueante y añaden timeout.
- `Spike_Hold_Motors` se aproxima con parada; M1 no tiene HOLD posicional.
- `Spike_Advance_For_distance` usa diámetro real de rueda y un factor de
  distancia configurable, no la constante de la rueda LEGO.
- Los factores y ganancias del LEGO no se reutilizan automáticamente.

No hace falta volver a flashear: la placa ya fue verificada byte por byte contra
`RosRobotControllerM4-extended-m1-pos-v4.hex`.

## Verificación realizada

En la Raspberry Pi 5 se configuró y compiló la variante completa con GCC 14.2.
Una ejecución desarmada de 10 segundos confirmó cámara RPi, Hailo10H y Oradar
simultáneos, sin abrir Hiwonder, con aproximadamente 29.95 FPS y salida limpia
al recibir `SIGINT`. Una ejecución con `HIWONDER_ARM=1`, sin placa ni
calibración, terminó antes de cargar el modelo y antes de crear el hilo de
misión, como exige la guarda de seguridad.
