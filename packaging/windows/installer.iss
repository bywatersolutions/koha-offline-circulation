; Inno Setup script for Koha Offline Circulation
;
; Expects a deploy directory prepared with windeployqt, containing
; KohaOfflineCirculation.exe and the Qt runtime. Compile with:
;
;   ISCC /DAppVersion=2.0.0 /DDeployDir=C:\path\to\deploy installer.iss

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef DeployDir
  #define DeployDir "..\..\deploy"
#endif

[Setup]
AppName=Koha Offline Circulation
AppVersion={#AppVersion}
AppPublisher=ByWater Solutions
AppPublisherURL=https://bywatersolutions.com/
DefaultDirName={autopf}\Koha Offline Circulation
DefaultGroupName=Koha Offline Circulation
OutputBaseFilename=KohaOfflineCirculation-{#AppVersion}-setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Compression=lzma2
SolidCompression=yes
LicenseFile=..\..\gpl.txt

[Files]
Source: "{#DeployDir}\*"; DestDir: "{app}"; Flags: recursesubdirs ignoreversion

[Icons]
Name: "{group}\Koha Offline Circulation"; Filename: "{app}\KohaOfflineCirculation.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\Koha Offline Circulation"; Filename: "{app}\KohaOfflineCirculation.exe"; WorkingDir: "{app}"

; windeployqt --compiler-runtime may drop the VC++ redistributable into the
; deploy tree, install it if so
#ifexist DeployDir + "\vc_redist.x64.exe"
[Run]
Filename: "{app}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Visual C++ runtime..."
#endif
