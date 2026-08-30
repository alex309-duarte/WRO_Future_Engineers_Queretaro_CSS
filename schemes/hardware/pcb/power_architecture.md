# Power Architecture - Expansion Board Rev A

## Overview
Arquitectura de potencia para placa de expansión robótica con batería 3S LiPo (12.6V max), Raspberry Pi 5, STM32F103RCT6, ICM-20948 IMU y motor JGB37-520 con encoder Hall.

## Power Entry & Protection Stage

### Input Connector
- **Type**: XT60 or XT30 connector
- **Voltage**: 3S LiPo (9V - 12.6V nominal)
- **Expected Current**: 8A peak (motor stall + RPi5 + peripherals)

### Protection Components

#### 1. Reverse Polarity Protection
**Option A (Recommended): P-Channel MOSFET**
- Component: SI2301DS or similar
- Advantage: ~0V drop, high efficiency
- Gate control: Simple resistor divider

**Option B: Schottky Diode**
- Component: MBRS340 (3A, 40V) or similar
- Disadvantage: ~0.3V drop, power loss
- Use if MOSFET complexity is not desired

#### 2. Transient Voltage Suppression (TVS)
- **Component**: SMBJ14A or SMAJ14A
- **Voltage**: 14V breakdown (protects against motor back-EMF spikes)
- **Placement**: Parallel to input, after reverse protection
- **Reason**: JGB37 motors generate voltage spikes >20V during braking/direction changes

#### 3. Resettable Fuse (PPTC)
- **Component**: MF-R080 or similar (0.8A hold, 1.6A trip) for logic
- **Component**: MF-R500 (5A hold, 10A trip) for motor line
- **Placement**: Series with input, before distribution

#### 4. Input Bulk Capacitor
- **Component**: 470µF - 1000µF electrolytic, 25V rated
- **Placement**: Immediately after protection stage
- **Purpose**: Absorb motor current transients and stabilize input

## Power Distribution Tree

### Main Power Rails

```
Battery (9-12.6V)
    │
    ├─── [Protection Stage] ───┐
    │                          │
    │                          ├─── VM (Motor Power) ──> Motor Drivers
    │                          │
    │                          └─── VIN_12V ──> Buck Converter
    │                                              │
    │                                              ├─── +5V (5A) ──> Raspberry Pi 5 + Hailo
    │                                              │                  │
    │                                              │                  └─── USB peripherals
    │                                              │
    │                                              └─── +5V ──> LDO Regulator
    │                                                            │
    │                                                            └─── +3V3 ──> STM32F103RCT6
    │                                                                          ICM-20948
    │                                                                          Logic level
    └─── GND (Star Ground Point)
```

### Regulator Specifications

#### Buck Converter: 12V → 5V
- **IC**: TPS54560 or equivalent
- **Output**: 5V @ 5A minimum
- **Efficiency**: >85% @ full load
- **Features**: 
  - Integrated FET
  - Adjustable switching frequency (500kHz recommended)
  - Enable pin for power sequencing
- **Input Cap**: 22µF ceramic + 100µF electrolytic
- **Output Cap**: 47µF ceramic + 100µF electrolytic
- **Inductor**: 10µH, 6A saturation current

#### LDO: 5V → 3.3V
- **IC**: AMS1117-3.3 or MCP1700-3302E
- **Output**: 3.3V @ 1A
- **Dropout**: <1.3V (AMS1117) or <0.35V (MCP1700)
- **Purpose**: Low-noise supply for MCU and IMU
- **Input Cap**: 10µF ceramic
- **Output Cap**: 10µF + 100nF ceramic (close to IC)

**Design Note**: The LDO fed from Buck output provides excellent noise filtering for sensitive analog components (IMU accelerometer/gyro). This two-stage approach prevents switching noise from corrupting sensor readings.

## Ground Architecture (Star Ground)

### Ground Planes Strategy
To prevent motor current return paths from causing voltage drops in logic ground:

1. **Power Ground (PGND)**: Heavy copper pour for motor drivers, battery return
2. **Analog Ground (AGND)**: Separate plane for IMU, ADC references
3. **Digital Ground (DGND)**: STM32, logic ICs
4. **Star Point**: All grounds connect at single point near battery connector

### PCB Layout Guidelines
- Keep motor driver ground traces wide (>2mm)
- Route AGND separately, connect to star point via ferrite bead or 0Ω resistor
- Place decoupling caps as close as possible to IC power pins
- Use ground vias liberally to connect top/bottom ground pours

## Decoupling Strategy

### Per-IC Requirements

#### STM32F103RCT6
- **VDD pins (1, 19, 32, 48, 64)**: 100nF ceramic each, close to pin
- **VDDA (pin 13)**: 100nF + 1µF ceramic + ferrite bead from VDD
- **VBAT (pin 1)**: 100nF ceramic
- **Bulk**: 10µF ceramic near MCU

#### ICM-20948 IMU
- **VDD (pin 13)**: 100nF + 1µF ceramic
- **VDDIO (pin 8)**: 100nF ceramic
- **Placement**: Caps within 5mm of IC

#### Raspberry Pi 5 Power Input
- **5V Rail**: 100µF electrolytic + 10µF ceramic at connector
- **USB Power**: Separate 47µF ceramic per USB port if available

## Motor Driver Considerations

### H-Bridge Selection Criteria
For JGB37-520 motor (3.2A stall current):

**Recommended IC: AM2861 (Primary Choice)**
- **Current Rating**: 5A continuous, 8A peak
- **Voltage Range**: 2.5V - 15V (perfect for 3S LiPo)
- **R_ds(on)**: ~0.15Ω (low heat dissipation)
- **Package**: SOP-8 (easy to solder, good thermal performance)
- **Features**:
  - Built-in protection: overcurrent, thermal shutdown, undervoltage lockout
  - PWM frequency up to 100kHz (low audible noise)
  - Logic-level inputs (3.3V compatible with STM32)
  - No external Schottky diodes needed (integrated body diodes)
- **Advantages over alternatives**:
  - **vs DRV8871**: Higher current rating (5A vs 3.6A), lower cost
  - **vs TB6612FNG**: Much higher current per channel (5A vs 1.2A)
  - **vs L298N**: Much lower dropout (~0.3V vs 2V), higher efficiency

**Alternative ICs** (if AM2861 not available):
- **DRV8871** (Single H-bridge, 3.6A continuous, 6.5A peak)
- **TB6612FNG** (Dual H-bridge, 1.2A continuous, 3.2A peak per channel)
- **BTS7960** (High-power, 43A, overkill but robust)

**Thermal Management**:
- Add thermal vias under driver IC pad (minimum 9 vias, 0.3mm diameter)
- Copper pour on bottom layer connected to GND for heat spreading
- Calculate power dissipation: P_loss = I²_rms × R_ds(on) × 2 (for H-bridge)
  - AM2861 @ 2A: (2A)² × 0.15Ω × 2 = **1.2W** (manageable without heatsink)
  - DRV8871 @ 2A: (2A)² × 0.3Ω × 2 = **2.4W** (needs better cooling)

### Motor Power Filtering
- **Bulk Cap**: 220µF electrolytic at motor driver VIN
- **Ceramic**: 1µF ceramic for high-frequency switching noise
- **Flyback Diodes**: Integrated in modern H-bridges, verify in datasheet

## Checklist de Potencia - Rev A

### Protection Stage
- [ ] Diodo Schottky o MOSFET de protección contra inversión de polaridad
- [ ] Diodo TVS de 14V-15V (SMBJ14A) para picos de motores
- [ ] Fusible PPTC (5A-10A) en línea de entrada
- [ ] Capacitor electrolítico grande (470µF - 1000µF, 25V) en entrada de 12V

### Regulators
- [ ] Buck converter 12V→5V con TPS54560 o equivalente (5A mínimo)
- [ ] Inductor de 10µH, 6A para Buck
- [ ] LDO 5V→3.3V con AMS1117-3.3 o MCP1700
- [ ] Capacitor de 10µF + 100nF a la entrada y salida de cada regulador

### MCU Power
- [ ] El STM32 tiene un capacitor de 100nF en cada pin VDD/VDDA
- [ ] VDDA con ferrite bead desde VDD
- [ ] Capacitor bulk de 10µF cerca del MCU

### IMU Power
- [ ] 100nF + 1µF en VDD del ICM-20948
- [ ] 100nF en VDDIO del ICM-20948
- [ ] Caps dentro de 5mm del IC

### Ground Strategy
- [ ] Star ground implementado cerca del conector de batería
- [ ] PGND separado para motores (trazas anchas >2mm)
- [ ] AGND para IMU conectado via ferrite bead o 0Ω
- [ ] Vias de ground abundantes entre capas

### Motor Driver
- [ ] H-bridge AM2861 seleccionado (5A continuo, 8A pico)
- [ ] Mínimo 9 vias térmicos (0.3mm) bajo pad del driver
- [ ] Copper pour en capa inferior para disipación térmica
- [ ] Capacitor bulk 220µF en VIN del driver
- [ ] Capacitor cerámico 1µF para ruido de conmutación
- [ ] Entradas PWM/DIR conectadas a pines timer del STM32 (PB8 para PWM)

## Component Selection Summary

| Function | Component | Package | Quantity | Notes |
|----------|-----------|---------|----------|-------|
| Reverse Protection | SI2301DS | SOT-23 | 1 | P-MOSFET |
| TVS Diode | SMBJ14A | DO-214AA | 1 | 14V breakdown |
| PPTC Fuse | MF-R500 | 1206 | 1 | 5A hold |
| Input Cap | 1000µF/25V | Electrolytic | 1 | Low ESR |
| Buck IC | TPS54560 | SOIC-8 | 1 | 5A output |
| Buck Inductor | 10µH/6A | SMD | 1 | Low DCR |
| LDO | AMS1117-3.3 | SOT-223 | 1 | 1A output |
| Decoupling | 100nF/50V | 0805 | 15+ | Ceramic X7R |
| Decoupling | 10µF/16V | 0805 | 8+ | Ceramic X7R |
| Motor Driver | DRV8871 | HTSSOP-8 | 1 | 3.6A continuous |

## Design Validation Tests

### Pre-Assembly
1. Verify polarity protection with multimeter (reverse battery should show open circuit)
2. Check TVS clamping voltage with bench supply and scope

### Post-Assembly
1. Power-up sequence test: Measure 12V → 5V → 3.3V rise times
2. Load test: Apply 2A load to 5V rail, verify <5% voltage drop
3. Motor transient test: Run motor forward/reverse, capture voltage spikes on scope
4. Thermal test: Run motor at stall for 10s, measure driver IC temperature
5. IMU noise test: Read accelerometer data while motor running, check for interference

## References
- AM2861 Datasheet: https://www.alldatasheet.com/datasheet-pdf/pdf/1132890/ANACHIP/AM2861.html
- TPS54560 Datasheet: https://www.ti.com/lit/ds/symlink/tps54560.pdf
- DRV8871 Datasheet (alternative): https://www.ti.com/lit/ds/symlink/drv8871.pdf
- STM32F103 Power Guidelines: AN2867
- ICM-20948 Hardware Integration: DS-000189

## Revision History
- **Rev A** (2026-03-15): Initial architecture definition
