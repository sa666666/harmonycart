; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=HarmonyCart
AppVerName=HarmonyCart 1.0
AppPublisher=Stephen Anthony
AppPublisherURL=http://atariage.com
AppSupportURL=http://atariage.com
AppUpdatesURL=http://atariage.com
DefaultDirName={pf}\HarmonyCart
DefaultGroupName=HarmonyCart
OutputBaseFilename="HarmonyCart-1.0-win32"
Compression=lzma
SolidCompression=yes

[Languages]
Name: "eng"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\Release\HarmonyCart.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\arm\*"; DestDir: "{app}\arm"; Flags: ignoreversion
Source: "..\*.txt"; DestDir: "{app}\docs"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\HarmonyCart"; Filename: "{app}\HarmonyCart.exe"; WorkingDir: "{app}"
Name: "{userdesktop}\HarmonyCart"; Filename: "{app}\HarmonyCart.exe"; WorkingDir: "{app}"; Tasks: desktopicon
;Name: "{group}\Documentation"; Filename: "{app}\docs\index.html"
Name: "{group}\Uninstall HarmonyCart"; Filename: "{uninstallexe}"

