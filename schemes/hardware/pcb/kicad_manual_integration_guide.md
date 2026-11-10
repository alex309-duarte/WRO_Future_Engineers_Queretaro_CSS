# KiCad Manual Integration Guide

> Design study: this expansion board is not mounted on the robot.

## Objective
Replace the placeholder blocks with real KiCad symbols and connect them to the existing hierarchical labels.

## Symbols to use
- **MCU**: `MCU_ST_STM32F1:STM32F103R_C-D-E_Tx`
- **IMU**: `Sensor_Motion:ICM-20948`
- **SWD/UART/Battery/Motor/Encoder**: `Connector_Generic:Conn_01x0X`
- **Motor driver (if available in a local or external library)**: `AM2861`

## Recommended workflow in KiCad

### 1. Open the project
- Open `hardware/pcb/future_engineers_robotics_expansion.kicad_pro`
- Go to the schematic editor

### 2. Integrate the real MCU
- Open the `mcu.kicad_sch` sheet
- Select the `U1` placeholder block and delete it
- Press `A` to add a symbol
- Search for `STM32F103R_C-D-E_Tx`
- Select `MCU_ST_STM32F1:STM32F103R_C-D-E_Tx`
- Place it in the center of the sheet
- Edit the properties:
  - **Reference**: `U1`
  - **Value**: `STM32F103RCT6`
  - **Footprint**: `Package_QFP:LQFP-64_10x10mm_P0.5mm`

### 3. Minimum MCU wiring
Connect these symbol pins to the hierarchical labels:

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

### 4. Integrate the real IMU
- Open `imu.kicad_sch`
- Delete the `U5` placeholder
- Press `A`
- Search for `ICM-20948`
- Select `Sensor_Motion:ICM-20948`
- Set:
  - **Reference**: `U5`
  - **Value**: `ICM-20948`
  - **Footprint**: `Sensor_Motion:InvenSense_QFN-24_3x3mm_P0.4mm`

### 5. Minimum IMU wiring
- `SDO/AD0` -> `IMU_AD0_SDO`
- `SDA/SDI` -> `IMU_SDA_SDI`
- `SCL/SCLK` -> `IMU_SCL_SCLK`
- `~CS` -> `IMU_nCS`
- `INT1` -> `IMU_INT`
- `VDDIO` -> `+3V3`
- `VDD` -> `+3V3`
- `GND` -> `GND`

### 6. Integrate the real connectors
#### Debug
- Use `Conn_01x04` or `Conn_02x03`
- Map: `3V3`, `SWDIO`, `SWCLK`, `GND`, optional `NRST`

#### UART
- Use `Conn_01x03`
- Map: `GND`, `UART_TX`, `UART_RX`

#### Battery
- Use `Conn_01x02`
- Map: `BAT+`, `GND`

#### Motor / Encoder
- Use `Conn_01x06`
- Map: `VM`, `M1+`, `M1-`, `H1A`, `H1B`, `GND`

### 7. Power sheet
In `power.kicad_sch` add:
- reverse polarity protection (`P-MOSFET` or Schottky)
- `SMBJ14A`
- 470uF-1000uF bulk capacitor
- 12V->5V buck
- 5V->3V3 LDO

### 8. Validation
- Run ERC after each important sheet
- Validate `mcu` first
- Then `imu`
- Then `debug/connectors/power`

## Practical tips
- Do not try to clear the whole ERC in a single pass
- Replace the symbol first, then connect the main signals
- If a symbol is not in the standard library, leave the block in place for now but update `Value`, `Footprint` and `Datasheet`
- Save after each important replacement
