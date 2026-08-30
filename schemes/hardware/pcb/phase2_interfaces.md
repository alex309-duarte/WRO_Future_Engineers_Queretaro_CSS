# Phase 2 Interfaces

## Global nets prepared
- `VM`
- `IN12V`
- `+5V`
- `+3V3`
- `GND`
- `DGND`
- `AGND`
- `VGND`
- `SWDIO`
- `SWCLK`
- `NRST`
- `BOOT0`
- `BOOT1`
- `UART_TX`
- `UART_RX`
- `IMU_INT`
- `IMU_SDA_SDI`
- `IMU_SCL_SCLK`
- `IMU_nCS`
- `IMU_AD0_SDO`
- `M1`
- `M1A`
- `M1B`
- `M1+`
- `M1-`
- `H1A`
- `H1B`

## MCU to sheets provisional pin planning
| Signal | MCU pin provisional | Direction at MCU | Origin sheet | Destination sheet |
|---|---|---|---|---|
| `+3V3` | `VDD_1..VDD_n` | input | `power` | `mcu` |
| `+5V` | `5V_AUX` | input | `power` | `mcu` |
| `GND` | `VSS_1..VSS_n` | bidirectional | `power` | `mcu` |
| `DGND` | `DGND_STAR` | bidirectional | `power` | `mcu` |
| `AGND` | `VSSA/AGND_REF` | bidirectional | `power` | `mcu` |
| `NRST` | `NRST` | bidirectional | `mcu` | `debug`, `user_io` |
| `BOOT0` | `BOOT0` | input | `debug` / `user_io` | `mcu` |
| `BOOT1` | `PB2 / BOOT1` | input | `debug` / `user_io` | `mcu` |
| `SWDIO` | `PA13` | bidirectional | `debug` | `mcu` |
| `SWCLK` | `PA14` | input | `debug` | `mcu` |
| `UART_TX` | `PA9 / USART1_TX` | output | `mcu` | `usb_uart`, `connectors` |
| `UART_RX` | `PA10 / USART1_RX` | input | `usb_uart`, `connectors` | `mcu` |
| `IMU_INT` | `PA3` | input | `imu` | `mcu` |
| `IMU_SDA_SDI` | `PB7 / I2C1_SDA` or `PA7 / SPI1_MOSI` | bidirectional | `mcu` | `imu` |
| `IMU_SCL_SCLK` | `PB6 / I2C1_SCL` or `PA5 / SPI1_SCK` | output | `mcu` | `imu` |
| `IMU_nCS` | `PA4 / SPI1_NSS` | output | `mcu` | `imu` |
| `IMU_AD0_SDO` | `PB5 / GPIO` or `PA6 / SPI1_MISO` | bidirectional | `imu` | `mcu` |
| `M1` | `PB8 / TIM4_CH3` | output | `mcu` | `motors_4x` |
| `M1A` | `PB0 / TIM3_CH3` | output | `mcu` | `motors_4x` |
| `M1B` | `PB1 / TIM3_CH4` | output | `mcu` | `motors_4x` |
| `H1A` | `PA0 / TIM2_CH1` | input | `motors_4x` / `connectors` | `mcu` |
| `H1B` | `PA1 / TIM2_CH2` | input | `motors_4x` / `connectors` | `mcu` |

## MCU to Motor 1 table
| Signal | MCU pin provisional | Direction | Origin sheet | Destination sheet |
|---|---|---|---|---|
| `M1` | `PB8 / TIM4_CH3` | output | `mcu` | `motors_4x` |
| `M1A` | `PB0 / TIM3_CH3` | output | `mcu` | `motors_4x` |
| `M1B` | `PB1 / TIM3_CH4` | output | `mcu` | `motors_4x` |
| `H1A` | `PA0 / TIM2_CH1` | input | `motors_4x` / `connectors` | `mcu` |
| `H1B` | `PA1 / TIM2_CH2` | input | `motors_4x` / `connectors` | `mcu` |
| `M1+` | `driver output` | output from motor stage | `motors_4x` | `connectors` |
| `M1-` | `driver output` | output from motor stage | `motors_4x` | `connectors` |

## MCU to IMU table
| Signal | MCU pin provisional | Direction | Origin sheet | Destination sheet |
|---|---|---|---|---|
| `IMU_INT` | `PA3` | input | `imu` | `mcu` |
| `IMU_SDA_SDI` | `PB7` or `PA7` | bidirectional | `mcu` | `imu` |
| `IMU_SCL_SCLK` | `PB6` or `PA5` | output | `mcu` | `imu` |
| `IMU_nCS` | `PA4` | output | `mcu` | `imu` |
| `IMU_AD0_SDO` | `PB5` or `PA6` | bidirectional | `imu` | `mcu` |

## Connector planning notes
- Battery / main power connector: pending `XT30` vs terminal block.
- SWD connector: 2.54 mm debug header baseline.
- UART external connector: simple `TX/RX/GND` header.
- Motor / encoder / compact peripherals: `JST-PH` baseline.

## Phase 2 boundaries by sheet
- `power`: power entry and derived rails only.
- `mcu`: central interface owner for SWD, UART, IMU, and Motor 1 control.
- `debug`: SWD + reset/boot access.
- `usb_uart`: optional serial bridge path only.
- `imu`: ICM20948 interface boundary only.
- `motors_4x`: treated as Motor 1-only motor stage boundary in this revision.
- `user_io`: minimum reset/boot access only.
- `connectors`: battery, UART, motor, and Hall/encoder external access.
