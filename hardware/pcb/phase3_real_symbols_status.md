# Phase 3: Real Symbols Integration - Status Report

## Objective
Replace visual placeholder blocks with actual KiCad library symbols and connect their pins to hierarchical labels.

## Symbols Located in KiCad Libraries

### MCU - STM32F103RCT6
- **Library**: `C:\Program Files\KiCad\9.0\share\kicad\symbols\MCU_ST_STM32F1.kicad_sym`
- **Symbol Name**: `STM32F103R_C-D-E_Tx`
- **Line Range**: 54113-55545 (1432 lines)
- **Footprint**: `Package_QFP:LQFP-64_10x10mm_P0.5mm`
- **Key Pins for Wiring**:
  - Pin 7: NRST
  - Pin 60: BOOT0
  - Pin 46: PA13 (SYS_JTMS-SWDIO) → SWDIO
  - Pin 49: PA14 (SYS_JTCK-SWCLK) → SWCLK
  - Pin 42: PA9 (USART1_TX) → UART_TX
  - Pin 43: PA10 (USART1_RX) → UART_RX
  - Pin 58: PB6 (I2C1_SCL) → IMU_SCL_SCLK
  - Pin 59: PB7 (I2C1_SDA) → IMU_SDA_SDI
  - Pin 61: PB8 (TIM4_CH3) → M1 PWM
  - Pin 26: PB0 → M1A (Hall A)
  - Pin 27: PB1 → M1B (Hall B)
  - Power: VDD (pins 1,13,19,32,48,64), VSS (pins 18,31,47,63), VDDA (pin 13), VSSA (pin 12)

### IMU - ICM-20948
- **Library**: `C:\Program Files\KiCad\9.0\share\kicad\symbols\Sensor_Motion.kicad_sym`
- **Symbol Name**: `ICM-20948`
- **Line Range**: 2948-3410 (462 lines)
- **Footprint**: `Sensor_Motion:InvenSense_QFN-24_3x3mm_P0.4mm`
- **Key Pins for Wiring**:
  - Pin 9: SDO/AD0 → IMU_AD0_SDO
  - Pin 24: SDA/SDI → IMU_SDA_SDI
  - Pin 23: SCL/SCLK → IMU_SCL_SCLK
  - Pin 22: ~CS → IMU_nCS
  - Pin 12: INT1 → IMU_INT
  - Pin 8: VDDIO → +3V3
  - Pin 13: VDD → +3V3
  - Pin 18: GND → GND

### Connectors
- **Library**: `C:\Program Files\KiCad\9.0\share\kicad\symbols\Connector_Generic.kicad_sym`
- **Available Symbols**:
  - `Conn_01x02` - For battery connector (VM+, GND)
  - `Conn_01x03` - For UART header (GND, TX, RX)
  - `Conn_01x04` - For SWD header (VCC, SWDIO, SWCLK, GND)
  - `Conn_01x06` - For motor/encoder connector (VM, M+, M-, HA, HB, GND)

## Current Status

### Completed
- ✅ Located all required symbols in KiCad standard libraries
- ✅ Identified exact pin mappings for MCU → peripherals
- ✅ Documented symbol locations and line ranges

### In Progress
- 🔄 Replacing `U1` placeholder in `mcu.kicad_sch` with real STM32 symbol
  - **Challenge**: Generated files had formatting issues causing KiCad parse errors
  - **Next Approach**: Manual editing using `multi_edit` tool with exact S-expression syntax

### Pending
- ⏳ Replace `U5` placeholder in `imu.kicad_sch` with real ICM-20948 symbol
- ⏳ Replace connector placeholders in `debug.kicad_sch`, `connectors.kicad_sch`
- ⏳ Add wire connections between symbol pins and hierarchical labels
- ⏳ Run ERC validation
- ⏳ Generate final report

## Recommended Next Steps

### Option A: Manual Symbol Integration (Recommended)
1. Use KiCad GUI to open `mcu.kicad_sch`
2. Delete the `U1` placeholder block
3. Add symbol from library: `MCU_ST_STM32F1:STM32F103R_C-D-E_Tx`
4. Place at coordinates (64.77, 101.6)
5. Wire pins to existing hierarchical labels
6. Repeat for IMU and connectors
7. Run ERC

### Option B: Programmatic Integration (Current Attempt)
1. Extract complete symbol S-expression from library
2. Embed into `lib_symbols` section of `.kicad_sch`
3. Update symbol instance to reference embedded symbol
4. Add wire elements connecting pins to labels
5. Validate syntax and run ERC

## Files Modified
- `hardware/pcb/generate_mcu_real.py` - Script to generate MCU schematic (had formatting issues)
- `hardware/pcb/generate_imu_real.py` - Script to generate IMU schematic (had formatting issues)
- `hardware/pcb/mcu.kicad_sch.bak` - Backup of original placeholder version

## Files Ready for Integration
- Original placeholder schematics are intact and valid
- Symbol definitions extracted from KiCad libraries
- Pin mapping tables documented

## Conclusion
The real symbols are located and ready. The challenge is properly embedding them into the `.kicad_sch` files with correct S-expression syntax. Manual integration via KiCad GUI is the safest path forward, or careful programmatic editing with exact format matching.
