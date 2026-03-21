# OpenClaw KiCad Host Bridge Usage

## Purpose
Puente local mínimo para que el host Windows pueda abrir y enfocar KiCad desde terminal, coordinado por Cascade/OpenClaw.

## Script
- `hardware/pcb/kicad_host_bridge.ps1`

## Supported actions
- `open_project`
- `focus_kicad`
- `status`
- `send_keys`
- `window_info`
- `click_relative`
- `capture_window`
- `click_and_type`
- `click_and_paste`

## Examples

### Open KiCad project
```powershell
powershell -ExecutionPolicy Bypass -File "c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\kicad_host_bridge.ps1" -Action open_project
```

### Focus KiCad window
```powershell
powershell -ExecutionPolicy Bypass -File "c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\kicad_host_bridge.ps1" -Action focus_kicad
```

### Check status
```powershell
powershell -ExecutionPolicy Bypass -File "c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\kicad_host_bridge.ps1" -Action status
```

### Send keys to KiCad
```powershell
powershell -ExecutionPolicy Bypass -File "c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\kicad_host_bridge.ps1" -Action send_keys -Keys "^s"
```

### Open the symbol dialog shortcut example
```powershell
powershell -ExecutionPolicy Bypass -File "c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\kicad_host_bridge.ps1" -Action send_keys -Keys "a"
```

### Read KiCad window bounds
```powershell
powershell -ExecutionPolicy Bypass -File "c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\kicad_host_bridge.ps1" -Action window_info
```

### Click relative to KiCad window
```powershell
powershell -ExecutionPolicy Bypass -File "c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\kicad_host_bridge.ps1" -Action click_relative -X 200 -Y 150
```

### Capture the targeted KiCad window
```powershell
powershell -ExecutionPolicy Bypass -File "c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\kicad_host_bridge.ps1" -Action capture_window -Target eeschema -OutputPath "c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\kicad_window_capture.png"
```

### Click and type in one execution
```powershell
powershell -ExecutionPolicy Bypass -File "c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\kicad_host_bridge.ps1" -Action click_and_type -Target eeschema -X 155 -Y 111 -Keys "STM32F103R_C-D-E_Tx" -DelayMs 1000 -OutputPath "c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\kicad_window_capture_after_click_and_type.png"
```

### Click and paste in one execution
```powershell
powershell -ExecutionPolicy Bypass -File "c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\kicad_host_bridge.ps1" -Action click_and_paste -Target eeschema -X 155 -Y 111 -Keys "STM32F103R_C-D-E_Tx" -DelayMs 1000 -OutputPath "c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\kicad_window_capture_after_click_and_paste.png"
```

## Next step
Después de validar apertura/enfoque, ampliar el bridge con acciones como:
- abrir hoja `mcu`
- enviar teclas controladas
- automatizar inserción de símbolos
- guardar proyecto

## Notes
Este bridge ya puede abrir, enfocar, enviar teclas, hacer clics relativos, capturar, ejecutar click+type y click+paste en un solo paso. Aun así, la automatización sigue siendo sensible al layout visual y conviene avanzar paso a paso.
