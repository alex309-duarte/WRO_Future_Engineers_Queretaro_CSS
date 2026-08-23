#!/usr/bin/env python3
"""Generate imu.kicad_sch with real ICM-20948 symbol and basic wiring."""

from pathlib import Path

# Read the real ICM-20948 symbol from KiCad library
lib_path = Path(r'C:\Program Files\KiCad\9.0\share\kicad\symbols\Sensor_Motion.kicad_sym')
lines = lib_path.read_text(encoding='utf-8').splitlines()

# Find and extract the ICM-20948 symbol
start_idx = next(i for i, l in enumerate(lines) if 'ICM-20948' in l and l.lstrip().startswith('(symbol '))
depth = 0
symbol_lines = []
for l in lines[start_idx:]:
    depth += l.count('(') - l.count(')')
    symbol_lines.append(l)
    if depth == 0:
        break

# Prepare the schematic with real IMU symbol
output = f"""(kicad_sch
	(version 20250114)
	(generator "eeschema")
	(generator_version "9.0")
	(uuid "d5e5dcb7-8af9-4676-bef3-6d0ae0d6a004")
	(paper "A4")
	(title_block
		(title "IMU - ICM20948")
	)
	(lib_symbols
		(symbol "Sensor_Motion:ICM-20948"
{chr(10).join('			' + l for l in symbol_lines[1:])}
	)
	(wire (pts (xy 78.74 73.66) (xy 91.44 73.66)) (stroke (width 0) (type default)) (uuid "w1"))
	(wire (pts (xy 78.74 76.2) (xy 91.44 76.2)) (stroke (width 0) (type default)) (uuid "w2"))
	(wire (pts (xy 78.74 78.74) (xy 91.44 78.74)) (stroke (width 0) (type default)) (uuid "w3"))
	(wire (pts (xy 78.74 81.28) (xy 91.44 81.28)) (stroke (width 0) (type default)) (uuid "w4"))
	(wire (pts (xy 78.74 86.36) (xy 91.44 86.36)) (stroke (width 0) (type default)) (uuid "w5"))
	(wire (pts (xy 40.64 63.5) (xy 53.34 63.5)) (stroke (width 0) (type default)) (uuid "w6"))
	(wire (pts (xy 40.64 96.52) (xy 53.34 96.52)) (stroke (width 0) (type default)) (uuid "w7"))
	(hierarchical_label "+3V3" (shape input) (at 40.64 63.5 180) (effects (font (size 1.27 1.27))) (uuid "h1"))
	(hierarchical_label "GND" (shape bidirectional) (at 40.64 96.52 180) (effects (font (size 1.27 1.27))) (uuid "h2"))
	(hierarchical_label "IMU_AD0_SDO" (shape bidirectional) (at 91.44 73.66 0) (effects (font (size 1.27 1.27))) (uuid "h3"))
	(hierarchical_label "IMU_SDA_SDI" (shape bidirectional) (at 91.44 76.2 0) (effects (font (size 1.27 1.27))) (uuid "h4"))
	(hierarchical_label "IMU_SCL_SCLK" (shape input) (at 91.44 78.74 0) (effects (font (size 1.27 1.27))) (uuid "h5"))
	(hierarchical_label "IMU_nCS" (shape input) (at 91.44 81.28 0) (effects (font (size 1.27 1.27))) (uuid "h6"))
	(hierarchical_label "IMU_INT" (shape output) (at 91.44 86.36 0) (effects (font (size 1.27 1.27))) (uuid "h7"))
	(symbol
		(lib_id "Sensor_Motion:ICM-20948")
		(at 66.04 81.28 0)
		(unit 1)
		(exclude_from_sim no)
		(in_bom yes)
		(on_board yes)
		(dnp no)
		(uuid "imu1")
		(property "Reference" "U5"
			(at 55.88 63.5 0)
			(effects (font (size 1.27 1.27)))
		)
		(property "Value" "ICM-20948"
			(at 73.66 99.06 0)
			(effects (font (size 1.27 1.27)))
		)
		(property "Footprint" "Sensor_Motion:InvenSense_QFN-24_3x3mm_P0.4mm"
			(at 66.04 106.68 0)
			(effects (font (size 1.27 1.27)) (hide yes))
		)
		(property "Datasheet" "http://www.invensense.com/wp-content/uploads/2016/06/DS-000189-ICM-20948-v1.3.pdf"
			(at 66.04 85.09 0)
			(effects (font (size 1.27 1.27)) (hide yes))
		)
		(property "Description" "9-axis IMU"
			(at 66.04 81.28 0)
			(effects (font (size 1.27 1.27)) (hide yes))
		)
		(pin "9" (uuid "p1"))
		(pin "24" (uuid "p2"))
		(pin "23" (uuid "p3"))
		(pin "22" (uuid "p4"))
		(pin "12" (uuid "p5"))
		(pin "8" (uuid "p6"))
		(pin "18" (uuid "p7"))
		(pin "13" (uuid "p8"))
		(instances
			(project "future_engineers_robotics_expansion"
				(path "/d5e5dcb7-8af9-4676-bef3-6d0ae0d6a004"
					(reference "U5")
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
out_path = Path(r'c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\imu.kicad_sch')
out_path.write_text(output, encoding='utf-8')
print(f"Generated {out_path} with real ICM-20948 symbol")
