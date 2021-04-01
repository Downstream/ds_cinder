
; TODO: Registry for Windows updates
; TODO: Test all switches and functions

#define GST GetEnv('GSTREAMER_1_0_ROOT_X86_64')
#define SYSTEMF GetEnv('SYSTEMROOT')
#define DS_PLATFORM GetEnv('DS_PLATFORM_090')

[Setup]
AppName={#APP_DISPLAY_NAME}
AppVersion={#APP_VERS}
AppPublisher=Downstream
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
DefaultDirName={pf}\Downstream\{#APP_NAME}
DefaultGroupName={#APP_DISPLAY_NAME}
OutputDir=install/build
OutputBaseFilename={#APP_NAME}-v{#APP_VERS}
SourceDir=../
UninstallDisplayIcon={app}\{#APP_EXE}
; OutputManifestFile={#APP_NAME}_log.txt

; For a lighter-weight install, disable all the wizard pages
DisableDirPage=yes
#ifdef USE_EXTRAS
DisableFinishedPage=no
#else
DisableFinishedPage=yes
#endif
DisableProgramGroupPage=yes
DisableReadyMemo=yes
DisableReadyPage=yes
DisableStartupPrompt=yes
DisableWelcomePage=yes

[Files]
Source: "vs2015/Release/*"; Excludes:"*\GPUCache\*,*.iobj,*.ipdb, *.pdb,{#APP_EXE}"; DestDir: "{app}"; Flags: recursesubdirs
Source: "settings/*"; Excludes:"*configuration.xml"; DestDir: "{app}/settings"; Flags: recursesubdirs
Source: "data/*"; DestDir: "{app}/data"; Flags: recursesubdirs

; Include the exe individually so it throws an error if it doesn't exist
Source: "vs2015/Release/{#APP_EXE}"; DestDir: "{app}"

#ifdef USE_APPHOST
Source: "install/DSAppHost/*"; DestDir: "{app}/DSAppHost/"; Flags: recursesubdirs
Source: "install/apphost.json"; DestDir: "{userdocs}\downstream\common\dsapphost\config"
#endif

#ifdef USE_DSNODE
Source: "install/DSNode/*"; DestDir: "{app}/DSNode/"; Flags: recursesubdirs
#endif

#ifexist "README.md"
Source: "README.md"; DestDir: "{app}"; Flags: isreadme
#endif

Source: "{#DS_PLATFORM}/install/msvcr100.dll"; DestDir: "{app}"
Source: "{#DS_PLATFORM}/install/msvcr120.dll"; DestDir: "{app}"
Source: "{#DS_PLATFORM}/.git/ORIG_HEAD"; DestDir: "{app}/data"; DestName: "ds_cinder_commit.txt"

#ifdef USE_GSTREAMER
Source: "{#GST}/bin/*.dll"; DestDir: "{app}/dll"
Source: "{#GST}/lib/gstreamer-1.0/*.dll"; DestDir: "{app}/dll/gst_plugins"
#endif

#ifdef USE_EXTRAS
Source: "{#DS_PLATFORM}/install/extras_installer.exe"; DestDir: "{app}"
#endif

#ifdef USE_EXTRAS
[Run]
Filename: "{app}\extras_installer.exe"; Description: "Install Extra Apps (Notepad++, 7zip, Google Chrome)"; Flags: postinstall skipifsilent unchecked
#endif

[Icons]
Name: "{group}\{#APP_DISPLAY_NAME}"; Filename: "{app}\{#APP_EXE}"
#ifndef SKIP_APP_ICON
Name: "{commondesktop}\{#APP_DISPLAY_NAME}"; Filename: "{app}\{#APP_EXE}"
#endif
#ifdef USE_APPHOST
Name: "{commondesktop}\{#APP_DISPLAY_NAME} DSAppHost"; Filename: "{app}\DSAppHost\DSAppHost.exe"
#endif

; In production will launch the app on system boot
#ifdef IS_PRODUCTION

; If we're using apphost, apphost will launch everything itself, so just launch apphost on startup
#ifdef USE_APPHOST
Name: "{commonstartup}\{#APP_NAME}-DSAppHost"; Filename: "{app}\DSAppHost\DSAppHost.exe"
#else
; No apphost, but yes for dsnode, so start that on system boot
#ifdef USE_DSNODE
Name: "{commonstartup}\{#APP_NAME}-DSNode"; Filename: "{app}\DSNode\DSNode.exe"
#endif
; No apphost, but start the main app on system boot
Name: "{commonstartup}\{#APP_NAME}"; Filename: "{app}\{#APP_EXE}"
#endif
#endif

[Registry]
; Sets the environment variable for pango text to look goodly
Root: HKCU; Subkey: "Environment"; ValueType: string; ValueName: "PANGOCAIRO_BACKEND"; ValueData: "fontconfig"; Flags: preservestringtype

#ifdef IS_PRODUCTION
; Disable the "program not responding" if this app crashed
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\Windows Error Reporting"; ValueType: dword; ValueName: "DontShowUI"; ValueData: "1"

#ifdef CMS_URL
; Set DS_BASEURL if the cms url has been defined
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: string; ValueName: "DS_BASEURL"; ValueData: "{#CMS_URL}"
#endif
#endif

; Check if DS_BASEURL environment variable is already set. If not, request a reboot
; Only checked if IS_PRODUCTION & CMS_URL are both set
#ifdef IS_PRODUCTION
#ifdef CMS_URL
[Code]
var
  CmsUrl: String;

function InitializeSetup(): Boolean;
begin
  RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment', 'DS_BASEURL', CmsUrl)

  Result := True;
end;

[Files]
; ^ This is a hack so that comments in the project installer after the include
;   don't break the code block :eyeroll:
#endif
#endif
