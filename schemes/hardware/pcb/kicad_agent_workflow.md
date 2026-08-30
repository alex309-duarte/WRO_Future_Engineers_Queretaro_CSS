# KiCad Agent Workflow

## Objetivo
Usar un agente GUI externo para operar KiCad, mientras Cascade prepara instrucciones exactas, archivos y validación ERC.

## Modelo operativo

### Roles
- **Cascade**
  - Decide qué cambiar
  - Edita archivos del proyecto cuando convenga
  - Define el cableado exacto y el símbolo correcto
  - Ejecuta ERC y revisa reportes

- **Agente GUI**
  - Abre KiCad
  - Navega entre hojas jerárquicas
  - Inserta símbolos desde la librería
  - Mueve, cablea y guarda cambios

## Qué debe poder hacer el agente GUI
En Windows, el agente debe tener capacidad de:
- abrir aplicaciones
- enfocar ventanas
- leer pantalla
- usar mouse y teclado
- opcionalmente tomar screenshots

## Flujo recomendado

### Fase 1. Lanzar KiCad
1. Abrir el proyecto:
   - `hardware/pcb/future_engineers_robotics_expansion.kicad_pro`
2. Entrar a Schematic Editor
3. Confirmar que se ven las hojas: `power`, `mcu`, `debug`, `imu`, `motors_4x`, `connectors`, `user_io`

### Fase 2. Hoja MCU
1. Abrir `mcu.kicad_sch`
2. Eliminar placeholder `U1`
3. Insertar símbolo:
   - `MCU_ST_STM32F1:STM32F103R_C-D-E_Tx`
4. Configurar campos:
   - `Reference = U1`
   - `Value = STM32F103RCT6`
   - `Footprint = Package_QFP:LQFP-64_10x10mm_P0.5mm`
5. Cablear:
   - `NRST -> NRST`
   - `BOOT0 -> BOOT0`
   - `PA13 -> SWDIO`
   - `PA14 -> SWCLK`
   - `PA9 -> UART_TX`
   - `PA10 -> UART_RX`
   - `PB6 -> IMU_SCL_SCLK`
   - `PB7 -> IMU_SDA_SDI`
   - `PB8 -> M1`
   - `PB0 -> M1A`
   - `PB1 -> M1B`
   - `VDD/VDDA/VBAT -> +3V3`
   - `VSS/VSSA -> GND/AGND`
6. Guardar

### Fase 3. Hoja IMU
1. Abrir `imu.kicad_sch`
2. Eliminar `U5` placeholder
3. Insertar símbolo:
   - `Sensor_Motion:ICM-20948`
4. Configurar campos:
   - `Reference = U5`
   - `Value = ICM-20948`
5. Cablear:
   - `SDO/AD0 -> IMU_AD0_SDO`
   - `SDA/SDI -> IMU_SDA_SDI`
   - `SCL/SCLK -> IMU_SCL_SCLK`
   - `~CS -> IMU_nCS`
   - `INT1 -> IMU_INT`
   - `VDDIO/VDD -> +3V3`
   - `GND -> GND`
6. Guardar

### Fase 4. Hoja motores_4x
1. Abrir `motors_4x.kicad_sch`
2. Mantener por ahora el bloque de `U6` o reemplazar si existe símbolo real del AM2861
3. Si no existe símbolo real:
   - conservar placeholder
   - ya está actualizado para reflejar `AM2861`
4. Cableado esperado:
   - `VM`
   - `GND/VGND`
   - `M1`
   - `M1A`
   - `M1B`
   - salidas `M1+`, `M1-`

### Fase 5. Conectores y debug
1. `debug.kicad_sch`
   - insertar header SWD real (`Conn_01x04` o `Conn_02x03`)
2. `connectors.kicad_sch`
   - batería: `Conn_01x02`
   - UART: `Conn_01x03`
   - motor/encoder: `Conn_01x06`

### Fase 6. Potencia
1. `power.kicad_sch`
2. Agregar:
   - protección por polaridad inversa
   - TVS `SMBJ14A`
   - capacitor bulk
   - buck 12V->5V
   - LDO 5V->3V3
3. Mantener coherencia con `power_architecture.md`

### Fase 7. Validación
1. Guardar todo
2. Ejecutar ERC
3. Corregir primero errores de símbolo faltante, pin sin conectar o tipos incompatibles

## Cómo coordinar con Cascade
Después de cada hoja, el agente GUI debe devolver uno de estos estados:
- `MCU listo`
- `IMU listo`
- `DEBUG listo`
- `CONNECTORS listo`
- `POWER listo`
- `Bloqueado: <motivo>`

Entonces Cascade:
- revisa siguiente paso
- ajusta mapeo si hace falta
- corre ERC cuando convenga

## Prompt corto para un agente GUI

```text
Actúa como operador de escritorio en Windows para KiCad 9.
No diseñes por tu cuenta.
Solo ejecuta exactamente las instrucciones técnicas que te iré dando.
Tu trabajo es:
1. abrir KiCad,
2. abrir el proyecto indicado,
3. entrar a la hoja indicada,
4. insertar el símbolo exacto indicado,
5. cablear solo las nets indicadas,
6. guardar,
7. reportar estado breve.

Reglas:
- No renombres nets por tu cuenta.
- No cambies hojas fuera de las indicadas.
- Si una librería no existe o no encuentras el símbolo, detente y reporta el bloqueo.
- Antes de borrar un placeholder, confirma visualmente la referencia del componente.
- Después de guardar cada hoja, responde solo con: "<HOJA> listo" o "Bloqueado: <motivo>".
```

## Limitación importante
Cascade no controla directamente la GUI de KiCad desde esta sesión. Necesita un agente de escritorio aparte o intervención tuya para los clics.
