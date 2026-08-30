# Procedencia de la variante

La base se copió el 2026-08-23 desde el estado de trabajo actual de:

```text
maker@Hailo10Wifi:
/home/maker/WRO_Future_Engineers_Queretaro_CSS/software_h10/cpp/object_detection_original_h10
```

Repositorio remoto:
`https://github.com/alex309-duarte/WRO_Future_Engineers_Queretaro_CSS.git`.
HEAD observado: `dea835645dc9deaecab655e65a95ad5601f5bd32` (`Open Challenge
improvements`).

El árbol de origen ya tenía cambios locales sin confirmar en
`object_detection.cpp`, `Oradar_S2L.*`, `common_var.h` y `spike.*`. La variante
parte deliberadamente de esos archivos de trabajo porque son los que producen
el ejecutable que usa el usuario. No se modificó ni limpió el origen.

La biblioteca Hiwonder se tomó del repositorio canónico de Windows:

```text
C:\Users\jesus\Documents\Jetson_cerebro\robotica\hiwonder_brick
```

`toolbox.cpp/.hpp` se copió a la variante para hacer observable
`terminating_main` desde el hilo de cámara y cerrar las colas al recibir
`Ctrl+C`, sin alterar el archivo común ni el programa original.
