' Export the SolidWorks assembly as STEP AP214 with colours (appearances).
' Run on Windows with SolidWorks installed:  cscript //nologo export_step_ap214.vbs "C:\path\Assem1.SLDASM" "C:\path\Assem1_AP214.step"
' Takes about 6 minutes for the full vehicle.
Option Explicit
Dim swApp, doc, r, src, dst
src = WScript.Arguments(0)
dst = WScript.Arguments(1)
Set swApp = CreateObject("SldWorks.Application")
swApp.Visible = True
swApp.SetUserPreferenceIntegerValue 75, 214     ' swStepAP = AP214 (AP203 carries no colours)
swApp.SetUserPreferenceToggle 787, True         ' swStepExportAppearances
Set doc = swApp.OpenDoc(src, 2)                 ' 2 = assembly
If doc Is Nothing Then
  WScript.Echo "Could not open " & src
  WScript.Quit 1
End If
doc.ResolveAllLightWeightComponents False
r = doc.SaveAs3(dst, 0, 2)
WScript.Echo "SaveAs3 result: " & r & " (0 = OK)"
