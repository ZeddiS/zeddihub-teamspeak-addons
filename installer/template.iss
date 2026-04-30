; Inno Setup template for ZeddiHub TeamSpeak addons.
; Each plugin's installer is generated from this template by build_installers.ps1
; with token substitution. The substituted .iss is then compiled by ISCC.
;
; Tokens:
;   {{PLUGIN_TITLE}}    — display name (e.g. "Poke Bot")
;   {{PLUGIN_SLUG}}     — folder name (e.g. "pokebot")
;   {{PLUGIN_VERSION}}  — version string (e.g. "1.2.0")
;   {{DLL_BASE}}        — DLL filename base (e.g. "zeddihub_pokebot")
;   {{DLL_DIR_API23}}   — full path to api23 DLL (or {empty} if missing)
;   {{DLL_DIR_API24}}
;   {{DLL_DIR_API25}}
;   {{DLL_DIR_API26}}
;   {{REPO_URL}}        — GitHub repo URL

#define MyAppName        "{{PLUGIN_TITLE}}"
#define MyAppSlug        "{{PLUGIN_SLUG}}"
#define MyAppVersion     "{{PLUGIN_VERSION}}"
#define MyAppPublisher   "ZeddiHub.eu"
#define MyAppURL         "{{REPO_URL}}"
#define DllBase          "{{DLL_BASE}}"

[Setup]
AppId={{{{PLUGIN_SLUG}}-zeddihub}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/issues
AppUpdatesURL={#MyAppURL}/releases
AppCopyright=Copyright (C) 2026 ZeddiHub.eu (zeddis.xyz)
DefaultDirName={userappdata}\TS3Client\plugins
DefaultGroupName={#MyAppName}
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputBaseFilename={#MyAppSlug}-Setup-v{#MyAppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
WizardSizePercent=110
SetupLogging=yes
UninstallDisplayName={#MyAppName} (TS3 Plugin)
UninstallDisplayIcon={app}\{#DllBase}_apiSELECTED_win64.dll

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "czech";   MessagesFile: "compiler:Languages\Czech.isl"

[Types]
Name: "auto";   Description: "Automatic detection (recommended)"
Name: "custom"; Description: "Choose TS3 version manually"; Flags: iscustom

[Components]
Name: "auto";  Description: "Auto-detect TS3 version (recommended)";       Types: auto custom; Flags: exclusive
Name: "api23"; Description: "TS3 client 3.5.0 (API 23)";                   Types: custom;      Flags: exclusive
Name: "api24"; Description: "TS3 client 3.5.1 - 3.5.5 (API 24)";           Types: custom;      Flags: exclusive
Name: "api25"; Description: "TS3 client 3.5.6 (API 25)";                   Types: custom;      Flags: exclusive
Name: "api26"; Description: "TS3 client 3.6.x and newer (API 26)";         Types: custom;      Flags: exclusive

[Files]
; Each API DLL is bundled but flagged "skipifsourcedoesntexist" so the
; installer compiles even if some variants are missing.
Source: "{{DLL_DIR_API23}}\{#DllBase}_api23_win64.dll"; DestDir: "{app}"; Components: api23; Flags: ignoreversion skipifsourcedoesntexist
Source: "{{DLL_DIR_API24}}\{#DllBase}_api24_win64.dll"; DestDir: "{app}"; Components: api24; Flags: ignoreversion skipifsourcedoesntexist
Source: "{{DLL_DIR_API25}}\{#DllBase}_api25_win64.dll"; DestDir: "{app}"; Components: api25; Flags: ignoreversion skipifsourcedoesntexist
Source: "{{DLL_DIR_API26}}\{#DllBase}_api26_win64.dll"; DestDir: "{app}"; Components: api26; Flags: ignoreversion skipifsourcedoesntexist

; For Auto component we copy the right DLL via [Code] section
Source: "{{DLL_DIR_API23}}\{#DllBase}_api23_win64.dll"; DestDir: "{tmp}"; DestName: "auto_api23.dll"; Components: auto; Flags: ignoreversion skipifsourcedoesntexist deleteafterinstall
Source: "{{DLL_DIR_API24}}\{#DllBase}_api24_win64.dll"; DestDir: "{tmp}"; DestName: "auto_api24.dll"; Components: auto; Flags: ignoreversion skipifsourcedoesntexist deleteafterinstall
Source: "{{DLL_DIR_API25}}\{#DllBase}_api25_win64.dll"; DestDir: "{tmp}"; DestName: "auto_api25.dll"; Components: auto; Flags: ignoreversion skipifsourcedoesntexist deleteafterinstall
Source: "{{DLL_DIR_API26}}\{#DllBase}_api26_win64.dll"; DestDir: "{tmp}"; DestName: "auto_api26.dll"; Components: auto; Flags: ignoreversion skipifsourcedoesntexist deleteafterinstall

[Run]

[UninstallDelete]
Type: files; Name: "{app}\{#DllBase}_api23_win64.dll"
Type: files; Name: "{app}\{#DllBase}_api24_win64.dll"
Type: files; Name: "{app}\{#DllBase}_api25_win64.dll"
Type: files; Name: "{app}\{#DllBase}_api26_win64.dll"

[Code]
function DetectTs3Api(): Integer;
var
    Ts3Path: String;
    QtCorePath: String;
    QtVersion: String;
begin
    Result := 25;  // sensible default — TS3 3.5.6

    // Try to find TS3 install path from registry
    if not RegQueryStringValue(HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\TeamSpeak 3 Client', 'InstallLocation', Ts3Path) then
        if not RegQueryStringValue(HKLM, 'SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\TeamSpeak 3 Client', 'InstallLocation', Ts3Path) then
            Ts3Path := 'C:\Program Files\TeamSpeak 3 Client';

    QtCorePath := Ts3Path + '\Qt5Core.dll';
    if FileExists(QtCorePath) then begin
        // GetVersionNumbersString returns "<major>.<minor>.<rev>.<build>"
        if GetVersionNumbersString(QtCorePath, QtVersion) then begin
            if Pos('5.15', QtVersion) > 0 then Result := 26
            else if Pos('5.12', QtVersion) > 0 then Result := 25
            else if Pos('5.9', QtVersion) > 0 then Result := 24
            else if Pos('5.6', QtVersion) > 0 then Result := 23;
        end;
    end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
    Api: Integer;
    SrcDll, DstDll: String;
begin
    if CurStep = ssPostInstall then begin
        // Auto component -> pick correct DLL based on detection
        if IsComponentSelected('auto') then begin
            Api := DetectTs3Api();
            SrcDll := ExpandConstant('{tmp}\auto_api') + IntToStr(Api) + '.dll';
            DstDll := ExpandConstant('{app}\') + ExpandConstant('{#DllBase}') + '_api' + IntToStr(Api) + '_win64.dll';
            if FileExists(SrcDll) then begin
                FileCopy(SrcDll, DstDll, False);
            end;
        end;
    end;
end;

function InitializeSetup(): Boolean;
var
    Ts3Running: Cardinal;
    ResultCode: Integer;
    UserChoice: Integer;
begin
    Result := True;
    // Check if TS3 is running — DLL would be locked.
    Exec('cmd.exe', '/c tasklist /FI "IMAGENAME eq ts3client_win64.exe" | find /I "ts3client_win64.exe" >nul', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
    if ResultCode = 0 then begin
        UserChoice := MsgBox(
            'TeamSpeak 3 is currently running.' + #13#10 + #13#10 +
            'The installer needs to update the plugin DLL, which is locked while TS3 runs.' + #13#10 + #13#10 +
            'Click YES to close TS3 automatically, or NO to abort.',
            mbConfirmation, MB_YESNO);
        if UserChoice = IDYES then begin
            Exec('cmd.exe', '/c taskkill /F /IM ts3client_win64.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
            Sleep(1500);
        end else begin
            Result := False;
        end;
    end;
end;

[Messages]
WelcomeLabel1=Welcome to {#MyAppName} {#MyAppVersion} setup
WelcomeLabel2=This will install the {#MyAppName} TeamSpeak 3 plugin for the current user.%n%nClick Next to continue.

[Icons]
Name: "{group}\{#MyAppName} on GitHub"; Filename: "{#MyAppURL}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
