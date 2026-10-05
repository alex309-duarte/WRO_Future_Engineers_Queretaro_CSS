# Reto 1 con la placa Hiwonder

En la rama `reto1-hiwonder`, el reto abierto de `main.cpp` corre sobre la placa
Hiwonder (STM32, firmware `pos-v8`) en lugar del hub SPIKE, sin relé. La lógica
del reto, el lidar Oradar y el botón no cambian.

## Qué cambia

| Archivo | Cambio |
|---|---|
| `hiwonder_config.h` | **Toda la calibración del robot** |
| `spike.h` | La API SPIKE del equipo, ahora sobre `spike_compat` |
| `spike_hiwonder.cpp` | Arranque de la placa y traducciones del hub |
| `hiwonder/` | Copia de la librería `hiwonder_brick` (estado del 2026-10-04) |
| `main.cpp` | Solo el arranque: `Spike_Hiwonder_Init()` en lugar del relé y el REPL |
| `spike.cpp` | Sigue en el árbol, pero ya no se compila |

Traducciones del adaptador, cubiertas por `make test`:

- `Spike_Reset_Gyro(int)` recibe **decigrados**, como `reset_yaw()` del hub
  (`Oradar_S2L_Wall_Slope()` devuelve el ángulo x10). La capa trabaja en grados.
- `Spike_Turn_For_Degrees(dir, speed, grados)` gira con la dirección a tope,
  como `turn()` del hub.
- `speed` 0..100 pasa a r/s en lazo cerrado (100 = 5.7 r/s), y
  `Spike_Advance_For_Degrees` mide la distancia con el encoder de M4.

## Compilar y probar

```bash
make        # my_lidar_app
make test   # pruebas offline con una placa falsa: no abre el puerto
```

## Antes de la primera corrida

Con las ruedas levantadas. La calibración de `hiwonder_config.h` es de banco, y
estas comprobaciones deciden si el reto gira hacia el lado correcto. Las
herramientas están en
`~/Jetson_cerebro/robotica/hiwonder_brick/hiwonder-analisis-estatico/tools/` y
usan el mismo puerto, así que `my_lidar_app` no debe estar corriendo.

1. **Signo del yaw.** Con `python3 imu_live.py`, gira el robot a mano a la
   derecha (sentido horario visto desde arriba): el yaw debe **bajar**, como en
   el hub. El reto depende de eso: tras girar a la derecha sigue con
   referencia -90.
2. **Lado de la dirección.** Con `python3 servo_hx12h.py --goto 300 --si-quiero-mover`
   (la derecha de `hiwonder_config.h`), las ruedas deben apuntar a la
   **derecha**. Vuelve al centro con `--goto 500`.
3. **Distancia.** `Spike_Advance_For_Degrees(100, 1350, 0)` debe recorrer lo
   mismo que con el hub. Si no, ajusta `kGradosSpikeATicks`.

Si fallan, sigue esta tabla:

| 1. Yaw | 2. Dirección | Qué hacer |
|---|---|---|
| bien | bien | Nada: la calibración es coherente |
| bien | al revés | Intercambia `kDireccionDerecha` y `kDireccionIzquierda` |
| al revés | cualquiera | El yaw de la placa está invertido respecto al hub. Se corrige en `hiwonder_brick`, no aquí, porque la capa lo lee directamente |

## Correr

```bash
WRO_MAX_RPS=1.5 ./my_lidar_app   # primeras pruebas, lento
./my_lidar_app                   # velocidad de competencia, 5.7 r/s
```

Al arrancar, centra la dirección y calibra el giroscopio durante 5 s: **el robot
debe estar quieto** hasta que parpadee el LED del botón. `WRO_MAX_RPS` reduce el
tope de velocidad sin recompilar; las distancias y los ángulos no cambian.

**Lo primero a vigilar en pista:** los bucles del lidar llaman a `Spike_Forward`
cada 2 ms, y el IMU llega a 20 Hz. El término D del PD solo ve un cambio en 1 de
cada ~25 llamadas, y lo aplica de golpe. Si la dirección vibra en las rectas, la
causa es esta.

## Parada de emergencia

El firmware **no tiene watchdog de comandos**: si el programa muere con el motor
andando, el motor sigue a la última velocidad. El adaptador lo para al salir y
ante una excepción, pero no si el proceso muere por una señal.

Ctrl+C **no detiene el robot**: corta los bucles del lidar, pero los giros y
avances de `main.cpp` siguen a ciegas. Un segundo Ctrl+C mata el proceso con el
motor como esté. Para pararlo:

```bash
python3 ~/Jetson_cerebro/robotica/hiwonder_brick/hiwonder-analisis-estatico/tools/motor_move.py --stop
```

o corta los 12 V.
