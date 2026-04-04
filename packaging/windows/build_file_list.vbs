
REM Build install file list from folder contents
REM Be sure to define excluded files and folders
REM Execute this script using cscript build_file_list.vbs
REM use !system 'cscript build_file_list.vbs'
REM use !include "install_files.nsh"
REM use !include "remove_files.nsh"

Set objInstFileToWrite = CreateObject("Scripting.FileSystemObject").OpenTextFile("install_files.nsh",2,true)
Set objDelFileToWrite = CreateObject("Scripting.FileSystemObject").OpenTextFile("remove_files.nsh",2,true)
name_exclusions = ";__pycache__;_yaml;mkdocs-1.6.1.dist-info;mkdocs_material-9.6.16.dist-info;mkdocs_material_extensions-1.3.1.dist-info;mkdocs_get_deps-0.2.0.dist-info;mkdocs_get_deps;mkdocs;PyYAML-6.0.2.dist-info;six-1.17.0.dist-info;yaml"

Set oFSO = CreateObject("Scripting.FileSystemObject")

cleanfolders = "" & vbCrLF & vbCrLF

BuildInstall "$INSTDIR\site-packages", "C:\program files\python313\lib\site-packages", name_exclusions

BuildInstall "$INSTDIR", "C:\Users\paulmckinney\Documents\python-3.13.2-embed-amd64", ""

REM some pyton dlls need to be in base install folder
REM Should remove.. we don't want items we dont' need in the install folder
REM BuildInstall "$INSTDIR", "C:\program files\python313\lib\site-packages\pywin32_system32", ""
REM BuildInstall "$INSTDIR", "C:\program files\python313\lib\site-packages\PyQt6\Qt6\bin", ""

REM BuildInstall "$INSTDIR\resources","C:\Qt\6.10.0\msvc2022_64\resources", ""
REM BuildInstall "$INSTDIR\qtwebengine_locales","C:\Qt\6.10.0\msvc2022_64\translations\qtwebengine_locales", ""

objDelFileToWrite.Write(cleanfolders)

Set oFSO = Nothing

objInstFileToWrite.Close
Set objInstFileToWrite = Nothing

Sub BuildInstall(byval baseinstall, byval basefolder, byval exclusions)
  WScript.StdOut.WriteLine "Searching folder " & basefolder

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

