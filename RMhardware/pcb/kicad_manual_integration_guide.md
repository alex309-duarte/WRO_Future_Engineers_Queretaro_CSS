# KiCad Manual Integration Guide

## Objetivo
Reemplazar los bloques placeholder por símbolos reales de KiCad y conectarlos a las etiquetas jerárquicas ya existentes.

## Símbolos a usar
- **MCU**: `MCU_ST_STM32F1:STM32F103R_C-D-E_Tx`
- **IMU**: `Sensor_Motion:ICM-20948`
- **SWD/UART/Batería/Motor/Encoder**: `Connector_Generic:Conn_01x0X`
- **Motor driver (si existe en librería local o externa)**: `AM2861`

## Flujo recomendado en KiCad

### 1. Abrir el proyecto
- Abre `hardware/pcb/future_engineers_robotics_expansion.kicad_pro`
- Entra al editor esquemático

### 2. Integrar el MCU real
- Abre la hoja `mcu.kicad_sch`
- Selecciona el bloque `U1` placeholder y elimínalo
- Presiona `A` para agregar símbolo
- Busca `STM32F103R_C-D-E_Tx`
- Selecciona `MCU_ST_STM32F1:STM32F103R_C-D-E_Tx`
- Colócalo en el centro de la hoja
- Edita propiedades:
  - **Reference**: `U1`
  - **Value**: `STM32F103RCT6`
  - **Footprint**: `Package_QFP:LQFP-64_10x10mm_P0.5mm`

### 3. Cableado mínimo del MCU
Conecta estos pines del símbolo a las etiquetas jerárquicas:

- `NRST` -> `NRST`
- `BOOT0` -> `BOOT0`
- `PA13 / SYS_JTMS-SWDIO` -> `SWDIO`
- `PA14 / SYS_JTCK-SWCLK` -> `SWCLK`
- `PA9 / USART1_TX` -> `UART_TX`
- `PA10 / USART1_RX` -> `UART_RX`
- `PB6 / I2C1_SCL` -> `IMU_SCL_SCLK`
- `PB7 / I2C1_SDA` -> `IMU_SDA_SDI`
- `PB8 / TIM4_CH3` -> `M1`
- `PB0` -> `M1A`
- `PB1` -> `M1B`
- `VDD`, `VDDA`, `VBAT` -> `+3V3`
- `VSS`, `VSSA` -> `GND` / `AGND`

### 4. Integrar la IMU real
- Abre `imu.kicad_sch`
- Elimina el placeholder `U5`
- Presiona `A`
- Busca `ICM-20948`
- Selecciona `Sensor_Motion:ICM-20948`
- Configura:
  - **Reference**: `U5`
  - **Value**: `ICM-20948`
  - **Footprint**: `Sensor_Motion:InvenSense_QFN-24_3x3mm_P0.4mm`

### 5. Cableado mínimo de la IMU
- `SDO/AD0` -> `IMU_AD0_SDO`
- `SDA/SDI` -> `IMU_SDA_SDI`
- `SCL/SCLK` -> `IMU_SCL_SCLK`
- `~CS` -> `IMU_nCS`
- `INT1` -> `IMU_INT`
- `VDDIO` -> `+3V3`
- `VDD` -> `+3V3`
- `GND` -> `GND`

### 6. Integrar conectores reales
#### Debug
- Usa `Conn_01x04` o `Conn_02x03`
- Mapea: `3V3`, `SWDIO`, `SWCLK`, `GND`, opcional `NRST`

#### UART
- Usa `Conn_01x03`
- Mapea: `GND`, `UART_TX`, `UART_RX`

#### Batería
- Usa `Conn_01x02`
- Mapea: `BAT+`, `GND`

#### Motor / Encoder
- Usa `Conn_01x06`
- Mapea: `VM`, `M1+`, `M1-`, `H1A`, `H1B`, `GND`

### 7. Power sheet
En `power.kicad_sch` agrega:
- protección por inversión (`P-MOSFET` o Schottky)
- `SMBJ14A`
- capacitor bulk 470uF-1000uF
- buck 12V->5V
- LDO 5V->3V3

### 8. Validación
- Ejecuta ERC después de cada hoja importante
- Primero valida `mcu`
- Luego `imu`
- Luego `debug/connectors/power`

## Consejos prácticos
- No intentes limpiar todo el ERC en una sola pasada
- Primero reemplaza símbolo, luego conecta señales principales
- Si un símbolo no está en librería estándar, deja temporalmente el bloque pero actualiza `Value`, `Footprint` y `Datasheet`
- Guarda después de cada reemplazo importante
