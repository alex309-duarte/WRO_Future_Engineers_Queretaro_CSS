# Procedencia del reto 2 con Hiwonder

Reescrito el 2026-10-18 sobre la rama `reto1-hiwonder`, en la Raspberry Pi 5
del robot (`maker@192.168.1.158`, Hailo-8, RPLidar S2L, placa Hiwonder con
firmware `pos-v8`).

## Misión

`object_detection.cpp` es el `object_detection.cpp` de
`../object_detection_original_h10` tal como está en el árbol de trabajo
(commit `e2bf80f` "WRO National competition" + los cambios locales de esa
carpeta), es decir, la misión completa del nacional: vueltas en los dos
sentidos, esquina, cubo del medio y estacionamiento. Se genera con
sustituciones puntuales sobre ese archivo (cada cambio lleva el comentario
`Hiwonder:`), así que `diff --strip-trailing-cr` contra el original muestra
sólo lo que cambia: el arranque de la placa, el apagado, el avance vigilado por
el lidar y tres esperas que no salían. El original no se modifica.

## Capa Hiwonder

No hay copia de la librería en esta carpeta. Se compilan directamente los
fuentes del reto 1, los mismos que corrieron en pista el 2026-10-18:

| Fuente | Qué aporta |
|---|---|
| `../../ondevice/hiwonder/` | `hiwonder_brick` (protocolo de la placa) y `spike_compat` (API SPIKE) |
| `../../ondevice/spike_hiwonder.cpp` | `Spike_Hiwonder_Init`: servo, centrado, calibración del giroscopio |
| `../../ondevice/parada_emergencia.cpp` | Ctrl+C / kill / caída = motores en COAST y servo sin par |
| `../../ondevice/hiwonder_config.h` | **Toda la calibración del robot** |
| `../../ondevice/Oradar_S2L.cpp` | Driver del RPLidar S2L (sin filtro de calidad) y buffer de 360° |

A `spike_compat` se le añadieron, sin cambiar lo que el reto 1 usa:
`Spike_Advance_For_Degrees` con reversa (`speed < 0`, como `ag_u()` del hub),
`Advance_For_Degrees_Until` (avance con condición de paro),
`Steer_Tire_Turn` y `Spike_Update_Config`. La placa falsa de las pruebas del
reto 1 ganó `cancel_motor_position`. Las 51 comprobaciones del reto 1 siguen
pasando.

## Modelo

`../../HailoModels/wro_h8_yolov8n_v4.hef`: YOLOv8n entrenado el 2026-10-18 con
el dataset completo de Roboflow (`wro-dataset-completo`, 9354 fotos con
aumento x3) y compilado con el Dataflow Compiler 3.34 para Hailo-8. Clases en
el orden `greenbox, redbox, xparking` (class_id 1, 2, 3 en `parse_nms_data`).
Evaluado en este mismo chip con las 477 fotos de validación de octubre:
mAP50 0.990 y decisión de la ronda 2 correcta en el 99.2 % (8 ms por foto).
`yolov8n_wro_hailo8.hef` (el anterior) sigue en la carpeta por si se quiere
comparar: `WRO_HIWONDER_MODEL=... ./run_hiwonder_h8.sh`.
