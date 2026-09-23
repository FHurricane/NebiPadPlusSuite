Unicode True
Name "NebiPad+ Suite"
OutFile "${OUTPUT}"
InstallDir "$LOCALAPPDATA\Programs\NebiPadPlusSuite"
RequestExecutionLevel user
SetCompressor /SOLID lzma

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "NebiPad+ Suite" SEC_MAIN
  SetOutPath "$INSTDIR"
  File /r "${STAGING}\*.*"
  CreateDirectory "$SMPROGRAMS\NebiPad+ Suite"
  CreateShortcut "$SMPROGRAMS\NebiPad+ Suite\NebiPad+ Suite.lnk" "$INSTDIR\NebiPadPlus.exe"
  CreateShortcut "$DESKTOP\NebiPad+ Suite.lnk" "$INSTDIR\NebiPadPlus.exe"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
  Delete "$DESKTOP\NebiPad+ Suite.lnk"
  Delete "$SMPROGRAMS\NebiPad+ Suite\NebiPad+ Suite.lnk"
  RMDir "$SMPROGRAMS\NebiPad+ Suite"
  RMDir /r "$INSTDIR"
SectionEnd
