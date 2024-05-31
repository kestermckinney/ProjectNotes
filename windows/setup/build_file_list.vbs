
REM Build install file list from folder contents
REM Be sure to define excluded files and folders
REM Execute this script using cscript build_file_list.vbs
REM use !system 'cscript build_file_list.vbs'
REM use !include "install_files.nsh"
REM use !include "remove_files.nsh"

Set objInstFileToWrite = CreateObject("Scripting.FileSystemObject").OpenTextFile("install_files.nsh",2,true)
Set objDelFileToWrite = CreateObject("Scripting.FileSystemObject").OpenTextFile("remove_files.nsh",2,true)
name_exclusions = ";__pycache__;PyInstaller;pyinstaller-5.8.0.dist-info;pyinstaller_hooks_contrib-2023.0.dist-info;pip-23.0.1.dist-info;pip;_pyinstaller_hooks_contrib;pywin32_system32;bin;"

Set oFSO = CreateObject("Scripting.FileSystemObject")

cleanfolders = "" & vbCrLF & vbCrLF

BuildInstall "$INSTDIR\site-packages", "C:\Users\Paul McKinney\AppData\Roaming\Python\Python311\site-packages", name_exclusions

BuildInstall "$INSTDIR", "C:\Users\Paul McKinney\Documents\python-3.11.3-embed-amd64", ""

REM some pyton dlls need to be in base install folder
BuildInstall "$INSTDIR", "C:\Users\Paul McKinney\AppData\Roaming\Python\Python311\site-packages\pywin32_system32", ""
BuildInstall "$INSTDIR", "C:\Users\Paul McKinney\AppData\Roaming\Python\Python311\site-packages\PyQt6\Qt6\bin", ""

objDelFileToWrite.Write(cleanfolders)

Set oFSO = Nothing

objInstFileToWrite.Close
Set objInstFileToWrite = Nothing

Sub BuildInstall(byval baseinstall, byval basefolder, byval exclusions)

  set oCurFolder = oFSO.GetFolder(basefolder)

  if Instr(exclusions, ";" & oCurFolder.Name & ";") = 0 and (oCurFolder.Files.Count + oCurFolder.SubFolders.Count) > 0 then
    objInstFileToWrite.Write( vbCrLF & "   ;Contains Files: " & oCurFolder.Files.Count & "  SubFolders: " & oCurFolder.SubFolders.Count  &  vbCrLF)
    objDelFileToWrite.Write( vbCrLF & "   ;Contains Files: " & oCurFolder.Files.Count & "  SubFolders: " & oCurFolder.SubFolders.Count  &  vbCrLF)

    objInstFileToWrite.Write("   SetOutPath """ & baseinstall & """" &  vbCrLF)
    objDelFileToWrite.Write("   ;Installed Path """ & baseinstall & """" &  vbCrLF)

    For Each oFile In oCurFolder.Files
      if Instr(exclusions, ";" & oFile.Name & ";") = 0 then
        objInstFileToWrite.Write("   File """ & oFile.Path & """" & vbCrLF)
        objDelFileToWrite.Write("   Delete """ & baseinstall & "\" & oFile.Name & """" & vbCrLF)
      end if
    Next      

    For Each oFolder In oFSO.GetFolder(basefolder).SubFolders
        BuildInstall baseinstall & "\" & oFolder.Name, basefolder & "\" & oFolder.Name, exclusions
    Next

    cleanfolders = cleanfolders & "   Delete """ & baseinstall & "\__pycache__\*.*""" &  vbCrLF
    cleanfolders = cleanfolders & "   RMDir """ & baseinstall & "\__pycache__""" &  vbCrLF
    cleanfolders = cleanfolders & "   RMDir """ & baseinstall & """" &  vbCrLF
  end if    
End Sub

