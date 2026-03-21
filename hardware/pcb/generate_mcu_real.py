#!/usr/bin/env python3
"""Generate mcu.kicad_sch with real STM32F103RCT6 symbol and basic wiring."""

from pathlib import Path

# Read the real STM32 symbol from KiCad library
lib_path = Path(r'C:\Program Files\KiCad\9.0\share\kicad\symbols\MCU_ST_STM32F1.kicad_sym')
lines = lib_path.read_text(encoding='utf-8').splitlines()

# Find and extract the STM32F103R_C-D-E_Tx symbol
start_idx = next(i for i, l in enumerate(lines) if '(symbol "STM32F103R_C-D-E_Tx"' in l)
depth = 0
symbol_lines = []
for l in lines[start_idx:]:
    depth += l.count('(') - l.count(')')
    symbol_lines.append(l)
    if depth == 0:
        break

# Prepare the schematic with real MCU symbol
output = f"""(kicad_sch
	(version 20250114)
	(generator "eeschema")
	(generator_version "9.0")
	(uuid "20d6dcb7-8af9-4676-bef3-6d0ae0d6a003")
	(paper "A4")
	(title_block
		(title "MCU - STM32F103RCT6")
	)
	(lib_symbols
		(symbol "MCU_ST_STM32F1:STM32F103R_C-D-E_Tx"
{chr(10).join('			' + l for l in symbol_lines[1:])}
	)
	(wire (pts (xy 83.82 63.5) (xy 96.52 63.5)) (stroke (width 0) (type default)) (uuid "w1"))
	(wire (pts (xy 83.82 66.04) (xy 96.52 66.04)) (stroke (width 0) (type default)) (uuid "w2"))
	(wire (pts (xy 83.82 81.28) (xy 96.52 81.28)) (stroke (width 0) (type default)) (uuid "w3"))
	(wire (pts (xy 83.82 78.74) (xy 96.52 78.74)) (stroke (width 0) (type default)) (uuid "w4"))
	(wire (pts (xy 83.82 73.66) (xy 96.52 73.66)) (stroke (width 0) (type default)) (uuid "w5"))
	(wire (pts (xy 83.82 71.12) (xy 96.52 71.12)) (stroke (width 0) (type default)) (uuid "w6"))
	(wire (pts (xy 45.72 101.6) (xy 33.02 101.6)) (stroke (width 0) (type default)) (uuid "w7"))
	(wire (pts (xy 45.72 99.06) (xy 33.02 99.06)) (stroke (width 0) (type default)) (uuid "w8"))
	(hierarchical_label "+3V3" (shape input) (at 33.02 38.1 180) (effects (font (size 1.27 1.27))) (uuid "h1"))
	(hierarchical_label "GND" (shape bidirectional) (at 33.02 147.32 180) (effects (font (size 1.27 1.27))) (uuid "h2"))
	(hierarchical_label "AGND" (shape bidirectional) (at 33.02 149.86 180) (effects (font (size 1.27 1.27))) (uuid "h3"))
	(hierarchical_label "NRST" (shape bidirectional) (at 33.02 101.6 180) (effects (font (size 1.27 1.27))) (uuid "h4"))
	(hierarchical_label "BOOT0" (shape input) (at 33.02 99.06 180) (effects (font (size 1.27 1.27))) (uuid "h5"))
	(hierarchical_label "SWDIO" (shape bidirectional) (at 96.52 63.5 0) (effects (font (size 1.27 1.27))) (uuid "h6"))
	(hierarchical_label "SWCLK" (shape input) (at 96.52 66.04 0) (effects (font (size 1.27 1.27))) (uuid "h7"))
	(hierarchical_label "UART_TX" (shape output) (at 96.52 78.74 0) (effects (font (size 1.27 1.27))) (uuid "h8"))
	(hierarchical_label "UART_RX" (shape input) (at 96.52 81.28 0) (effects (font (size 1.27 1.27))) (uuid "h9"))
	(hierarchical_label "IMU_SDA_SDI" (shape bidirectional) (at 96.52 73.66 0) (effects (font (size 1.27 1.27))) (uuid "h10"))
	(hierarchical_label "IMU_SCL_SCLK" (shape output) (at 96.52 71.12 0) (effects (font (size 1.27 1.27))) (uuid "h11"))
	(symbol
		(lib_id "MCU_ST_STM32F1:STM32F103R_C-D-E_Tx")
		(at 64.77 101.6 0)
		(unit 1)
		(exclude_from_sim no)
		(in_bom yes)
		(on_board yes)
		(dnp no)
		(uuid "mcu1")
		(property "Reference" "U1"
			(at 64.77 50.8 0)
			(effects (font (size 1.27 1.27)))
		)
		(property "Value" "STM32F103RCT6"
			(at 64.77 53.34 0)
			(effects (font (size 1.27 1.27)))
		)
		(property "Footprint" "Package_QFP:LQFP-64_10x10mm_P0.5mm"
			(at 49.53 147.32 0)
			(effects (font (size 1.27 1.27)) (justify right) (hide yes))
		)
		(property "Datasheet" "https://www.st.com/resource/en/datasheet/stm32f103rc.pdf"
			(at 64.77 101.6 0)
			(effects (font (size 1.27 1.27)) (hide yes))
		)
		(property "Description" "STM32F103RCT6 Main MCU"
			(at 64.77 101.6 0)
			(effects (font (size 1.27 1.27)) (hide yes))
		)
		(pin "7" (uuid "p1"))
		(pin "60" (uuid "p2"))
		(pin "46" (uuid "p3"))
		(pin "49" (uuid "p4"))
		(pin "42" (uuid "p5"))
		(pin "43" (uuid "p6"))
		(pin "58" (uuid "p7"))
		(pin "59" (uuid "p8"))
		(pin "1" (uuid "p9"))
		(pin "13" (uuid "p10"))
		(pin "19" (uuid "p11"))
		(pin "32" (uuid "p12"))
		(pin "48" (uuid "p13"))
		(pin "64" (uuid "p14"))
		(pin "12" (uuid "p15"))
		(pin "18" (uuid "p16"))
		(pin "31" (uuid "p17"))
		(pin "47" (uuid "p18"))
		(pin "63" (uuid "p19"))
		(instances
			(project "future_engineers_robotics_expansion"
				(path "/20d6dcb7-8af9-4676-bef3-6d0ae0d6a003"
					(reference "U1")
					(unit 1)
				)
			)
		)
	)
	(sheet_instances
		(path "/"
			(page "1")
		)
	)
	(embedded_fonts no)
)
"""

# Write the output
out_path = Path(r'c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\mcu.kicad_sch')
out_path.write_text(output, encoding='utf-8')
print(f"Generated {out_path} with real STM32 symbol")
