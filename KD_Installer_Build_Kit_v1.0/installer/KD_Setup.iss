#define MyAppName "KD Core"
#define MyAppVersion "2.0.0"
#define MyAppExeName "KDCore.exe"
#define MyAppSha256 GetSHA256OfFile(AddBackslash(SourcePath) + "..\dist\KDCore.exe")

[Setup]
AppId={{7FBBDFE6-1C4D-4A4F-8E89-3DA7B0B9F26A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=KD Core
DefaultDirName={autopf}\KD Core
DefaultGroupName=KD Core
DisableProgramGroupPage=yes
DisableWelcomePage=yes
DisableDirPage=yes
DisableReadyPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
MinVersion=10.0
WizardStyle=modern light
WizardSizePercent=100
WizardImageFile=assets\wizard_large.png
WizardSmallImageFile=assets\wizard_small.png
WizardImageBackColor=#263238
WizardSmallImageBackColor=#263238
WizardBackColor=#263238
SetupIconFile=..\resources\KD.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName=KD Core
OutputDir=..\dist\installer
OutputBaseFilename=KD_Core_Setup
Compression=lzma2/ultra64
SolidCompression=yes
CloseApplications=yes
RestartApplications=no
VersionInfoVersion=2.0.0.0
VersionInfoCompany=KD Core
VersionInfoDescription=Instalator KD Core
VersionInfoProductName=KD Core
VersionInfoProductVersion=2.0.0

[Languages]
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"

[Messages]
SetupWindowTitle=KD Core — Instalator
SetupAppTitle=KD Core
ConfirmUninstall=KD Core zostanie usunięty z tego komputera. Kontynuować?
UninstalledAll=KD Core został pomyślnie usunięty.

[Tasks]
Name: "desktopicon"; Description: "Utwórz skrót na pulpicie"; GroupDescription: "Dodatkowe skróty:"; Flags: unchecked

[Files]
Source: "assets\ui_logo.png"; Flags: dontcopy
Source: "assets\ui_bg.png"; Flags: dontcopy
Source: "assets\ui_tab_indicator.png"; Flags: dontcopy
Source: "assets\ui_card.png"; Flags: dontcopy
Source: "assets\ui_card_list.png"; Flags: dontcopy
Source: "assets\ui_field.png"; Flags: dontcopy
Source: "assets\ui_check.png"; Flags: dontcopy
Source: "assets\ui_warn.png"; Flags: dontcopy
Source: "assets\btn_install.png"; Flags: dontcopy
Source: "assets\btn_install_hover.png"; Flags: dontcopy
Source: "assets\btn_install_down.png"; Flags: dontcopy
Source: "assets\btn_reinstall.png"; Flags: dontcopy
Source: "assets\btn_reinstall_hover.png"; Flags: dontcopy
Source: "assets\btn_reinstall_down.png"; Flags: dontcopy
Source: "assets\btn_uninstall.png"; Flags: dontcopy
Source: "assets\btn_uninstall_hover.png"; Flags: dontcopy
Source: "assets\btn_uninstall_down.png"; Flags: dontcopy
Source: "assets\btn_backup.png"; Flags: dontcopy
Source: "assets\btn_backup_hover.png"; Flags: dontcopy
Source: "assets\btn_backup_down.png"; Flags: dontcopy
Source: "assets\btn_finish.png"; Flags: dontcopy
Source: "assets\btn_finish_hover.png"; Flags: dontcopy
Source: "assets\btn_finish_down.png"; Flags: dontcopy
Source: "assets\btn_restore.png"; Flags: dontcopy
Source: "assets\btn_restore_hover.png"; Flags: dontcopy
Source: "assets\btn_restore_down.png"; Flags: dontcopy
Source: "assets\btn_browse.png"; Flags: dontcopy
Source: "assets\btn_browse_hover.png"; Flags: dontcopy
Source: "assets\btn_browse_down.png"; Flags: dontcopy
Source: "assets\opt_repair.png"; Flags: dontcopy
Source: "assets\opt_repair_sel.png"; Flags: dontcopy
Source: "assets\opt_clean.png"; Flags: dontcopy
Source: "assets\opt_clean_sel.png"; Flags: dontcopy
Source: "..\dist\KDCore.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\KD Core"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\KD Core"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Odinstaluj KD Core"; Filename: "{uninstallexe}"

[Code]
const
  UninstallKey = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{7FBBDFE6-1C4D-4A4F-8E89-3DA7B0B9F26A}_is1';
  LegacySettingsKey = 'Software\Kombajn Diagnostyczny';
  SettingsKey = 'Software\KD Core';
  AppVersion = '2.0.0';
  AppExe = 'KDCore.exe';
  ExpectedHash = '{#MyAppSha256}';

  MODE_INSTALL = 1;
  MODE_REINSTALL = 2;
  MODE_UNINSTALL = 3;

  TAB_INSTALL = 0;
  TAB_REINSTALL = 1;
  TAB_UNINSTALL = 2;
  TAB_BACKUPS = 3;

  SCREEN_TABS = 0;
  SCREEN_PROGRESS = 1;
  SCREEN_FINISH = 2;

  HOVER_NONE = 0;
  HOVER_TAB_INSTALL = 1;
  HOVER_TAB_REINSTALL = 2;
  HOVER_TAB_UNINSTALL = 3;
  HOVER_TAB_BACKUPS = 4;
  HOVER_PRIMARY = 10;
  HOVER_BROWSE = 11;
  HOVER_RESTORE = 12;
  HOVER_MIN = 13;
  HOVER_CLOSE = 14;
  HOVER_CANCEL = 15;

  SW_MINIMIZE = 6;
  SWP_NOSIZE = 1;
  SWP_NOZORDER = 4;
  SWP_NOACTIVATE = 16;
  VK_LBUTTON = 1;

  ClrBg = $00323826;
  ClrSecondary = $004F4737;
  ClrSecondaryHover = $005E5440;
  ClrBorder = $00645A45;
  ClrAccent = $00FAD481;
  ClrText = $00FCF6EA;
  ClrMuted = $00C5BBAA;
  ClrSuccess = $00A7D6A5;
  ClrError = $009A9AEF;
  ClrField = $00423B2E;
  ClrTrack = $004F4737;

type
  TKdPoint = record
    X, Y: Longint;
  end;
  TKdRect = record
    Left, Top, Right, Bottom: Longint;
  end;

function GetCursorPos(var lpPoint: TKdPoint): Boolean;
  external 'GetCursorPos@user32.dll stdcall';
function GetWindowRect(hWnd: Longint; var lpRect: TKdRect): Boolean;
  external 'GetWindowRect@user32.dll stdcall';
function SetWindowPos(hWnd, hWndInsertAfter: Longint; X, Y, cx, cy: Integer; uFlags: Longint): Boolean;
  external 'SetWindowPos@user32.dll stdcall';
function GetAsyncKeyState(vKey: Integer): Integer;
  external 'GetAsyncKeyState@user32.dll stdcall';
function ShowWindow(hWnd: Longint; nCmdShow: Integer): Boolean;
  external 'ShowWindow@user32.dll stdcall';
function SetTimer(hWnd, nIDEvent, uElapse, lpTimerFunc: Longword): Longword;
  external 'SetTimer@user32.dll stdcall';
function KillTimer(hWnd, nIDEvent: Longword): Boolean;
  external 'KillTimer@user32.dll stdcall';
function DwmSetWindowAttribute(hWnd, Attr: Longword; var Value: Integer; Size: Longword): Integer;
  external 'DwmSetWindowAttribute@dwmapi.dll stdcall delayload';
function CreateRoundRectRgn(X1, Y1, X2, Y2, W, H: Integer): Longword;
  external 'CreateRoundRectRgn@gdi32.dll stdcall';
function SetWindowRgn(hWnd, hRgn: Longword; Redraw: Boolean): Integer;
  external 'SetWindowRgn@user32.dll stdcall';

var
  MainPage: TWizardPage;
  Shell, TopBar, TopBorder: TPanel;
  BGImage, Logo: TBitmapImage;
  NameLabel, VersionLabel, MinLabel, CloseLabel: TLabel;
  TitleLabel, DescLabel, MetaLabel: TLabel;
  TabLabels: array[0..3] of TLabel;
  TabUnderline: TBitmapImage;
  TabDivider: TLabel;
  InstallPanel, ReinstallPanel, UninstallPanel, BackupsPanel, ProgressPanel, FinishPanel: TPanel;

  InstallCard: TBitmapImage;
  InstallSysCaption, InstallSysValue, InstallFreeCaption, InstallFreeValue: TLabel;
  InstallVerCaption, InstallVerValue, InstallReqCaption, InstallReqValue: TLabel;
  InstallPathLabel, InstallReqLabel, InstallNoteLabel, InstallHintLabel: TLabel;
  InstallField: TBitmapImage;
  InstallDirEdit: TNewEdit;
  BrowseButton: TBitmapButton;
  DesktopCheck: TNewCheckBox;

  ReinstallCard: TBitmapImage;
  ReVersionCaption, ReVersionValue, ReFolderCaption, ReFolderValue, ReFilesCaption, ReFilesValue: TLabel;
  ReOptionsLabel, ReinstallNoteLabel, ReinstallHintLabel: TLabel;
  RepairButton, CleanButton: TBitmapButton;

  UninstallCard: TBitmapImage;
  UnVersionCaption, UnVersionValue, UnFolderCaption, UnFolderValue, UnFilesCaption, UnFilesValue: TLabel;
  KeepSettingsCheck: TNewCheckBox;
  UninstallNoteLabel, UninstallHintLabel: TLabel;

  BackupIntroLabel, BackupEmptyLabel, BackupPathLabel, BackupNoteLabel, RefreshLabel: TLabel;
  BackupCard: TBitmapImage;
  BackupList: TNewListBox;
  RestoreButton: TBitmapButton;

  ProgressTitleLabel, ProgressDescLabel, ProgressHintLabel: TLabel;
  ProgressCard: TBitmapImage;
  ProgOpCaption, ProgOpValue, ProgPathCaption, ProgPathValue: TLabel;

  FinishIcon: TBitmapImage;
  FinishHeadingLabel, FinishDetailLabel: TLabel;
  RunCheck: TNewCheckBox;

  FooterBorder, ProgressTrack, ProgressFill: TPanel;
  StatusLabel, PercentLabel, CancelLabel: TLabel;
  PrimaryButton: TBitmapButton;

  Installed, InstalledExeOk, CleanInstall, UninstallStarted, PostInstallOk, DesktopWanted: Boolean;
  CleanOptionSelected, PrimaryDown, ShellReady, DragActive, PrevLeftDown, RunStarted: Boolean;
  StartupPageFixed: Boolean;
  KeepSettingsWanted: Boolean;
  Mode, ActiveTab, ScreenState, HoverTarget: Integer;
  UninstallTicks, UninstallPercent, LastExitCode: Integer;
  InstalledDir, InstalledVersion, InstalledUninstaller, InstalledExeState, ChosenInstallDir, BackupDir: String;
  BackupEntries: TStringList;
  UninstallMarker, UninstallCmdFile, LastUninstallError: String;
  PrimaryAsset, PrimaryVisualKey, BrowseVisualKey, RestoreVisualKey: String;
  TimerID, ProgressTimerID: Longword;
  DragOffsetX, DragOffsetY: Integer;

function YesNo(B: Boolean): String;
begin
  if B then
    Result := 'yes'
  else
    Result := 'no';
end;

function HasParam(const Value: String): Boolean;
var
  I: Integer;
begin
  Result := False;
  for I := 1 to ParamCount do
    if CompareText(ParamStr(I), Value) = 0 then
    begin
      Result := True;
      Exit;
    end;
end;

function NormalizeDir(const Dir: String): String;
begin
  Result := Trim(Dir);
  while (Length(Result) > 3) and (Result[Length(Result)] = '\') do
    Delete(Result, Length(Result), 1);
end;

function IsDriveRoot(const Dir: String): Boolean;
begin
  Result := (Length(Dir) = 3) and (Dir[2] = ':') and (Dir[3] = '\');
end;

function FreeSpaceText(const Path: String): String;
var
  FreeBytes, TotalBytes: Int64;
  Root: String;
begin
  Root := ExtractFileDrive(Path);
  if Root = '' then
    Root := ExpandConstant('{sd}');
  if GetSpaceOnDisk64(Root + '\', FreeBytes, TotalBytes) then
  begin
    if FreeBytes >= (1024 * 1024 * 1024) then
      Result := IntToStr(FreeBytes div (1024 * 1024 * 1024)) + ' GB wolne'
    else
      Result := IntToStr(FreeBytes div (1024 * 1024)) + ' MB wolne';
  end
  else
    Result := 'wolne miejsce: nieznane';
end;

function BuildMeta(const Dir: String): String;
var
  Target: String;
begin
  Target := Dir;
  if Target = '' then
    Target := ExpandConstant('{autopf}\KD Core');
  Result := 'Windows ' + GetWindowsVersionString + ' x64  •  ' + FreeSpaceText(Target);
end;

function ReadInstallFrom(const Root: Integer; var Dir, Ver, Uninst: String): Boolean;
begin
  Dir := '';
  Ver := '';
  Uninst := '';
  Result := RegKeyExists(Root, UninstallKey);
  if Result then
  begin
    if not RegQueryStringValue(Root, UninstallKey, 'InstallLocation', Dir) then
      Dir := '';
    if not RegQueryStringValue(Root, UninstallKey, 'DisplayVersion', Ver) then
      Ver := '';
    if not RegQueryStringValue(Root, UninstallKey, 'UninstallString', Uninst) then
      Uninst := '';
  end;
end;

function DetectInstallation: Boolean;
var
  Dir, Ver, Uninst, ExePath: String;
  Found: Boolean;
begin
  InstalledDir := '';
  InstalledVersion := '';
  InstalledUninstaller := '';
  InstalledExeOk := False;
  InstalledExeState := 'brak instalacji';
  Found := False;

  if ReadInstallFrom(HKEY_LOCAL_MACHINE_64, Dir, Ver, Uninst) then
  begin
    InstalledDir := Dir;
    InstalledVersion := Ver;
    InstalledUninstaller := Uninst;
    Found := True;
  end
  else if ReadInstallFrom(HKEY_LOCAL_MACHINE_32, Dir, Ver, Uninst) then
  begin
    InstalledDir := Dir;
    InstalledVersion := Ver;
    InstalledUninstaller := Uninst;
    Found := True;
  end
  else if ReadInstallFrom(HKEY_CURRENT_USER, Dir, Ver, Uninst) then
  begin
    InstalledDir := Dir;
    InstalledVersion := Ver;
    InstalledUninstaller := Uninst;
    Found := True;
  end;

  if InstalledDir = '' then
  begin
    if FileExists(ExpandConstant('{autopf}\KD Core\' + AppExe)) then
      InstalledDir := ExpandConstant('{autopf}\KD Core')
    else if FileExists(ExpandConstant('{localappdata}\KD Core\' + AppExe)) then
      InstalledDir := ExpandConstant('{localappdata}\KD Core');
  end;

  if (InstalledDir <> '') and not Found then
  begin
    Found := True;
    InstalledVersion := 'nieznana';
  end;

  Installed := Found;
  if Installed then
  begin
    InstalledDir := NormalizeDir(InstalledDir);
    if InstalledVersion = '' then
      InstalledVersion := 'nieznana';
    if InstalledDir <> '' then
    begin
      ExePath := AddBackslash(InstalledDir) + AppExe;
      if not FileExists(ExePath) then
      begin
        InstalledExeOk := False;
        InstalledExeState := 'brak pliku KDCore.exe';
      end
      else
      begin
        try
          if CompareText(GetSHA256OfFile(ExePath), ExpectedHash) = 0 then
          begin
            InstalledExeOk := True;
            InstalledExeState := 'kompletne';
          end
          else
          begin
            InstalledExeOk := False;
            InstalledExeState := 'wymagają naprawy';
          end;
        except
          InstalledExeOk := False;
          InstalledExeState := 'nie można sprawdzić';
        end;
      end;
    end
    else
    begin
      InstalledDir := ExpandConstant('{autopf}\KD Core');
      InstalledExeState := 'brak pliku KDCore.exe';
    end;
  end;
  Result := Installed;
end;

function VerifyInstalledExe: Boolean;
var
  Hash: String;
begin
  Result := False;
  if not FileExists(ExpandConstant('{app}\' + AppExe)) then
    Exit;
  try
    Hash := GetSHA256OfFile(ExpandConstant('{app}\' + AppExe));
    Result := CompareText(Hash, ExpectedHash) = 0;
  except
    Result := False;
  end;
end;

function FindUninstaller(var Uninstaller: String): Boolean;
var
  Candidate, Dir: String;
  P: Integer;
  FindRec: TFindRec;
begin
  Uninstaller := '';
  Candidate := Trim(InstalledUninstaller);
  if Copy(Candidate, 1, 1) = '"' then
  begin
    Delete(Candidate, 1, 1);
    P := Pos('"', Candidate);
    if P > 0 then
      Candidate := Copy(Candidate, 1, P - 1);
  end
  else
  begin
    P := Pos(' ', Candidate);
    if P > 0 then
      Candidate := Copy(Candidate, 1, P - 1);
  end;
  Candidate := Trim(Candidate);
  if (Candidate <> '') and FileExists(Candidate) then
  begin
    Uninstaller := Candidate;
    Result := True;
    Exit;
  end;

  Dir := InstalledDir;
  if Dir = '' then
    Dir := ExpandConstant('{autopf}\KD Core');
  Candidate := AddBackslash(Dir) + 'unins000.exe';
  if FileExists(Candidate) then
  begin
    Uninstaller := Candidate;
    Result := True;
    Exit;
  end;

  if FindFirst(AddBackslash(Dir) + 'unins*.exe', FindRec) then
  begin
    try
      repeat
        if (FindRec.Attributes and FILE_ATTRIBUTE_DIRECTORY) = 0 then
        begin
          Uninstaller := AddBackslash(Dir) + FindRec.Name;
          Break;
        end;
      until not FindNext(FindRec);
    finally
      FindClose(FindRec);
    end;
  end;
  Result := Uninstaller <> '';
end;

function FormatBackupBase(const Base: String): String;
begin
  { settings_yyyy-mm-dd_hh-nn-ss.reg }
  if (Length(Base) >= 32) and (Copy(Base, 10, 1) = '2') then
    Result := Copy(Base, 10, 10) + '  ' + Copy(Base, 21, 2) + ':' + Copy(Base, 24, 2)
  else
    Result := Base;
end;

function NewLabel(AParent: TPanel; const ACaption: String; X, Y, W, H, ASize: Integer;
  const AFont: String; AColor: TColor): TLabel;
begin
  Result := TLabel.Create(AParent);
  Result.Parent := AParent;
  Result.AutoSize := False;
  Result.SetBounds(ScaleX(X), ScaleY(Y), ScaleX(W), ScaleY(H));
  Result.Caption := ACaption;
  Result.WordWrap := False;
  Result.Transparent := True;
  Result.Layout := tlCenter;
  Result.Font.Name := AFont;
  Result.Font.Size := ASize;
  Result.Font.Color := AColor;
end;

function NewWrappedLabel(AParent: TPanel; const ACaption: String; X, Y, W, H, ASize: Integer;
  const AFont: String; AColor: TColor): TLabel;
begin
  Result := NewLabel(AParent, ACaption, X, Y, W, H, ASize, AFont, AColor);
  Result.WordWrap := True;
  Result.Layout := tlTop;
end;

function NewCard(AParent: TPanel; X, Y, W, H: Integer; const AFileName: String): TBitmapImage;
begin
  Result := TBitmapImage.Create(AParent);
  Result.Parent := AParent;
  Result.SetBounds(ScaleX(X), ScaleY(Y), ScaleX(W), ScaleY(H));
  Result.Stretch := True;
  Result.BackColor := clNone;
  Result.PngImage.LoadFromFile(ExpandConstant('{tmp}\') + AFileName);
end;

function NewButton(AParent: TPanel; X, Y, W, H: Integer; const AFileName: String): TBitmapButton;
begin
  Result := TBitmapButton.Create(AParent);
  Result.Parent := AParent;
  Result.SetBounds(ScaleX(X), ScaleY(Y), ScaleX(W), ScaleY(H));
  Result.Stretch := True;
  Result.BackColor := clNone;
  Result.Cursor := crHand;
  Result.PngImage.LoadFromFile(ExpandConstant('{tmp}\') + AFileName);
end;

function NewCheck(AParent: TPanel; X, Y, W, H: Integer; const ACaption: String;
  AChecked: Boolean): TNewCheckBox;
begin
  Result := TNewCheckBox.Create(AParent);
  Result.Parent := AParent;
  Result.SetBounds(ScaleX(X), ScaleY(Y), ScaleX(W), ScaleY(H));
  Result.Caption := ACaption;
  Result.Checked := AChecked;
  Result.Color := ClrBg;
  Result.Font.Name := 'Segoe UI';
  Result.Font.Size := 9;
  Result.Font.Color := ClrText;
  Result.Cursor := crHand;
end;

function NewEdit(AParent: TPanel; X, Y, W, H: Integer): TNewEdit;
begin
  Result := TNewEdit.Create(AParent);
  Result.Parent := AParent;
  Result.SetBounds(ScaleX(X), ScaleY(Y), ScaleX(W), ScaleY(H));
  Result.AutoSize := False;
  Result.BorderStyle := bsNone;
  Result.Color := ClrField;
  Result.Font.Name := 'Segoe UI';
  Result.Font.Size := 10;
  Result.Font.Color := ClrText;
end;

procedure SetButtonPng(B: TBitmapButton; const AFileName: String);
var
  Path: String;
begin
  if B = nil then
    Exit;
  Path := ExpandConstant('{tmp}\') + AFileName;
  if not FileExists(Path) then
  begin
    Exit;
  end;
  try
    B.PngImage.LoadFromFile(Path);
    B.Invalidate;
  except
  end;
end;

procedure SetBtnVisual(B: TBitmapButton; const ABase: String; AHovered, ADown: Boolean;
  var AKey: String);
var
  Suffix, NewKey: String;
begin
  if (B = nil) or (ABase = '') then
    Exit;
  Suffix := '';
  if AHovered then
  begin
    if ADown then
      Suffix := '_down'
    else
      Suffix := '_hover';
  end;
  NewKey := ABase + Suffix + '.png';
  if AKey = NewKey then
    Exit;
  AKey := NewKey;
  SetButtonPng(B, NewKey);
end;

function PointInLabel(X, Y: Integer; L: TLabel): Boolean;
begin
  Result := (L <> nil) and L.Visible and
            (X >= L.Left) and (X < L.Left + L.Width) and
            (Y >= L.Top) and (Y < L.Top + L.Height);
end;

function PointInWindow(const P: TKdPoint; W: TWinControl): Boolean;
var
  R: TKdRect;
begin
  Result := False;
  if (W = nil) or (not W.Showing) then
    Exit;
  if GetWindowRect(W.Handle, R) then
    Result := (P.X >= R.Left) and (P.X < R.Right) and
              (P.Y >= R.Top) and (P.Y < R.Bottom);
end;

procedure SetStatus(const AText: String; AKind: Integer);
begin
  if StatusLabel = nil then
    Exit;
  StatusLabel.Caption := AText;
  if AKind = 1 then
    StatusLabel.Font.Color := ClrSuccess
  else if AKind = 2 then
    StatusLabel.Font.Color := ClrError
  else if AKind = 3 then
    StatusLabel.Font.Color := ClrAccent
  else
    StatusLabel.Font.Color := ClrMuted;
end;

procedure RefreshHover;
var
  I: Integer;
begin
  if (TabLabels[0] = nil) or (MinLabel = nil) then
    Exit;

  for I := 0 to 3 do
  begin
    if I = ActiveTab then
      TabLabels[I].Font.Color := ClrAccent
    else if HoverTarget = HOVER_TAB_INSTALL + I then
      TabLabels[I].Font.Color := ClrText
    else
      TabLabels[I].Font.Color := ClrMuted;
  end;

  if HoverTarget = HOVER_MIN then
  begin
    MinLabel.Color := ClrSecondaryHover;
    MinLabel.Font.Color := ClrText;
  end
  else
  begin
    MinLabel.Color := ClrSecondary;
    MinLabel.Font.Color := ClrMuted;
  end;

  if HoverTarget = HOVER_CLOSE then
  begin
    CloseLabel.Color := ClrSecondaryHover;
    CloseLabel.Font.Color := ClrError;
  end
  else
  begin
    CloseLabel.Color := ClrSecondary;
    CloseLabel.Font.Color := ClrMuted;
  end;

  if CancelLabel <> nil then
  begin
    if HoverTarget = HOVER_CANCEL then
      CancelLabel.Font.Color := ClrAccent
    else
      CancelLabel.Font.Color := ClrMuted;
  end;

  if (PrimaryButton <> nil) and PrimaryButton.Showing then
    SetBtnVisual(PrimaryButton, PrimaryAsset, HoverTarget = HOVER_PRIMARY, PrimaryDown, PrimaryVisualKey);
  if (BrowseButton <> nil) and BrowseButton.Showing then
    SetBtnVisual(BrowseButton, 'btn_browse', HoverTarget = HOVER_BROWSE, False, BrowseVisualKey);
  if (RestoreButton <> nil) and RestoreButton.Showing then
    SetBtnVisual(RestoreButton, 'btn_restore', HoverTarget = HOVER_RESTORE, False, RestoreVisualKey);
end;

procedure ShellTimerProc(Arg1, Arg2, Arg3, Arg4: Longword);
var
  P: TKdPoint;
  R: TKdRect;
  X, Y, NewHover: Integer;
  LeftDown, NewDown: Boolean;
begin
  if (not ShellReady) or (WizardForm = nil) or (not WizardForm.Visible) then
    Exit;

  if (not StartupPageFixed) and (WizardForm.CurPageID = wpWelcome) then
  begin
    StartupPageFixed := True;
    WizardForm.NextButton.OnClick(nil);
    Exit;
  end;

  if not GetCursorPos(P) then
    Exit;
  if not GetWindowRect(WizardForm.Handle, R) then
    Exit;

  LeftDown := (GetAsyncKeyState(VK_LBUTTON) and $8000) <> 0;
  X := P.X - R.Left;
  Y := P.Y - R.Top;

  if LeftDown and (not PrevLeftDown) and
     (X >= 0) and (X < ScaleX(560)) and (Y >= 0) and (Y < ScaleY(44)) and
     (not PointInLabel(X, Y, MinLabel)) and (not PointInLabel(X, Y, CloseLabel)) then
  begin
    DragActive := True;
    DragOffsetX := X;
    DragOffsetY := Y;
  end;

  if DragActive and LeftDown then
  begin
    SetWindowPos(WizardForm.Handle, 0, P.X - DragOffsetX, P.Y - DragOffsetY, 0, 0,
      SWP_NOSIZE or SWP_NOZORDER or SWP_NOACTIVATE);
    GetWindowRect(WizardForm.Handle, R);
    X := P.X - R.Left;
    Y := P.Y - R.Top;
  end;
  if not LeftDown then
    DragActive := False;

  NewHover := HOVER_NONE;
  if PointInLabel(X, Y, MinLabel) then
    NewHover := HOVER_MIN
  else if PointInLabel(X, Y, CloseLabel) then
    NewHover := HOVER_CLOSE
  else if TabLabels[0].Visible and PointInLabel(X, Y, TabLabels[0]) then
    NewHover := HOVER_TAB_INSTALL
  else if TabLabels[1].Visible and PointInLabel(X, Y, TabLabels[1]) then
    NewHover := HOVER_TAB_REINSTALL
  else if TabLabels[2].Visible and PointInLabel(X, Y, TabLabels[2]) then
    NewHover := HOVER_TAB_UNINSTALL
  else if TabLabels[3].Visible and PointInLabel(X, Y, TabLabels[3]) then
    NewHover := HOVER_TAB_BACKUPS
  else if PointInWindow(P, PrimaryButton) and PrimaryButton.Visible then
    NewHover := HOVER_PRIMARY
  else if PointInWindow(P, BrowseButton) and BrowseButton.Visible then
    NewHover := HOVER_BROWSE
  else if PointInWindow(P, RestoreButton) and RestoreButton.Visible then
    NewHover := HOVER_RESTORE
  else if PointInLabel(X, Y, CancelLabel) then
    NewHover := HOVER_CANCEL;

  NewDown := (NewHover = HOVER_PRIMARY) and LeftDown;
  if (NewHover <> HoverTarget) or (NewDown <> PrimaryDown) then
  begin
    HoverTarget := NewHover;
    PrimaryDown := NewDown;
    RefreshHover;
  end;

  PrevLeftDown := LeftDown;
end;

function CurrentInstallDir: String;
begin
  if (InstallDirEdit <> nil) and (Trim(InstallDirEdit.Text) <> '') then
    Result := NormalizeDir(InstallDirEdit.Text)
  else if ChosenInstallDir <> '' then
    Result := ChosenInstallDir
  else
    Result := ExpandConstant('{autopf}\KD Core');
end;

function InstalledSummary: String;
begin
  if Installed then
  begin
    Result := InstalledVersion;
    if InstalledExeOk then
      Result := Result + '  •  pliki kompletne'
    else
      Result := Result + '  •  ' + InstalledExeState;
  end
  else
    Result := 'nie zainstalowano';
end;

procedure UpdateHeader;
begin
  if (TitleLabel = nil) or (ScreenState <> SCREEN_TABS) then
    Exit;
  case ActiveTab of
    TAB_INSTALL:
      begin
        TitleLabel.Caption := 'Zainstaluj KD Core';
        DescLabel.Caption := 'Nowa instalacja lub aktualizacja na tym komputerze.';
      end;
    TAB_REINSTALL:
      begin
        TitleLabel.Caption := 'Napraw lub zainstaluj ponownie';
        DescLabel.Caption := 'Przywróć pliki programu, zachowując ustawienia i raporty.';
      end;
    TAB_UNINSTALL:
      begin
        TitleLabel.Caption := 'Odinstaluj KD Core';
        DescLabel.Caption := 'Usuń program. Ustawienia możesz zachować na później.';
      end;
    TAB_BACKUPS:
      begin
        TitleLabel.Caption := 'Kopie zapasowe';
        DescLabel.Caption := 'Zabezpiecz lub przywróć ustawienia KD Core.';
      end;
  end;
  MetaLabel.Caption := BuildMeta(CurrentInstallDir);
end;

procedure UpdateInstallPanel;
begin
  if InstallSysValue = nil then
    Exit;
  InstallSysValue.Caption := 'Windows ' + GetWindowsVersionString + '  x64';
  InstallFreeValue.Caption := FreeSpaceText(CurrentInstallDir);
  InstallVerValue.Caption := InstalledSummary;
  if Installed then
    InstallNoteLabel.Caption := 'KD Core jest już zainstalowany w tej wersji. Instalacja odświeży pliki programu i zachowa ustawienia.'
  else
    InstallNoteLabel.Caption := 'Program zostanie zapisany w wybranym folderze. Ustawienia i raporty w Dokumentach nie zostaną usunięte.';
end;

procedure UpdateReinstallPanel;
begin
  if ReVersionValue = nil then
    Exit;
  if Installed then
  begin
    ReVersionValue.Caption := InstalledVersion;
    ReFolderValue.Caption := InstalledDir;
    ReFilesValue.Caption := InstalledExeState;
    ReinstallNoteLabel.Caption := 'Naprawa zachowuje ustawienia KD Core oraz raporty. Czysta instalacja usuwa ustawienia, ale pozostawia raporty.';
  end
  else
  begin
    ReVersionValue.Caption := 'brak instalacji';
    ReFolderValue.Caption := '—';
    ReFilesValue.Caption := '—';
    ReinstallNoteLabel.Caption := 'KD Core nie jest zainstalowany. Użyj zakładki Instaluj, aby wykonać pierwszą instalację.';
  end;
end;

procedure UpdateUninstallPanel;
begin
  if UnVersionValue = nil then
    Exit;
  if Installed then
  begin
    UnVersionValue.Caption := InstalledVersion;
    UnFolderValue.Caption := InstalledDir;
    UnFilesValue.Caption := InstalledExeState;
    UninstallNoteLabel.Caption := 'Raporty w folderze Dokumenty\KD Core\Raporty nie są usuwane. Przed odinstalowaniem program zostanie zamknięty.';
  end
  else
  begin
    UnVersionValue.Caption := 'brak instalacji';
    UnFolderValue.Caption := '—';
    UnFilesValue.Caption := '—';
    UninstallNoteLabel.Caption := 'KD Core nie jest zainstalowany na tym komputerze.';
  end;
end;

procedure RefreshBackupList; forward;

procedure UpdateBackupPanel;
begin
  if BackupList = nil then
    Exit;
  BackupPathLabel.Caption := 'Folder: ' + BackupDir;
  RefreshBackupList;
end;

procedure UpdatePrimaryButton;
begin
  if PrimaryButton = nil then
    Exit;
  case ActiveTab of
    TAB_INSTALL:   PrimaryAsset := 'btn_install';
    TAB_REINSTALL: PrimaryAsset := 'btn_reinstall';
    TAB_UNINSTALL: PrimaryAsset := 'btn_uninstall';
    TAB_BACKUPS:   PrimaryAsset := 'btn_backup';
  else
    PrimaryAsset := 'btn_install';
  end;
  PrimaryVisualKey := '';
  SetBtnVisual(PrimaryButton, PrimaryAsset, HoverTarget = HOVER_PRIMARY, False, PrimaryVisualKey);
end;

procedure RefreshTabContent;
begin
  if ScreenState <> SCREEN_TABS then
    Exit;
  UpdateHeader;
  UpdateInstallPanel;
  UpdateReinstallPanel;
  UpdateUninstallPanel;
  UpdateBackupPanel;
  UpdatePrimaryButton;
end;

procedure SetScreenState(AState: Integer);
begin
  ScreenState := AState;

  InstallPanel.Visible := AState = SCREEN_TABS;
  ReinstallPanel.Visible := (AState = SCREEN_TABS) and (ActiveTab = TAB_REINSTALL);
  UninstallPanel.Visible := (AState = SCREEN_TABS) and (ActiveTab = TAB_UNINSTALL);
  BackupsPanel.Visible := (AState = SCREEN_TABS) and (ActiveTab = TAB_BACKUPS);
  InstallPanel.Visible := (AState = SCREEN_TABS) and (ActiveTab = TAB_INSTALL);
  ProgressPanel.Visible := AState = SCREEN_PROGRESS;
  FinishPanel.Visible := AState = SCREEN_FINISH;

  TabLabels[0].Visible := AState = SCREEN_TABS;
  TabLabels[1].Visible := AState = SCREEN_TABS;
  TabLabels[2].Visible := AState = SCREEN_TABS;
  TabLabels[3].Visible := AState = SCREEN_TABS;
  TabUnderline.Visible := AState = SCREEN_TABS;
  TabDivider.Visible := AState = SCREEN_TABS;

  ProgressTrack.Visible := AState = SCREEN_PROGRESS;
  PercentLabel.Visible := AState = SCREEN_PROGRESS;
  CancelLabel.Visible := AState <> SCREEN_FINISH;
  PrimaryButton.Visible := AState <> SCREEN_PROGRESS;
  RunCheck.Visible := False;

  if AState = SCREEN_TABS then
  begin
    UpdateHeader;
    SetStatus('Gotowe. Wybierz działanie.', 0);
    PercentLabel.Caption := '';
    ProgressFill.Width := 0;
    PrimaryAsset := '';
    UpdatePrimaryButton;
  end;
end;

procedure SwitchTab(ATab: Integer);
begin
  if (ScreenState <> SCREEN_TABS) or (UninstallStarted) then
    Exit;
  ActiveTab := ATab;
  SetScreenState(SCREEN_TABS);
  UpdateHeader;
  TabUnderline.Left := ScaleX(28 + ATab * 112);
  if ATab = TAB_BACKUPS then
    TabUnderline.Width := ScaleX(112)
  else
    TabUnderline.Width := ScaleX(104);
  RefreshTabContent;
  RefreshHover;
end;

procedure PrimaryClick(Sender: TObject); forward;
procedure RestoreClick(Sender: TObject); forward;
procedure RefreshClick(Sender: TObject); forward;
procedure UpdateFinishPanel; forward;

procedure TabInstallClick(Sender: TObject);
begin
  SwitchTab(TAB_INSTALL);
end;

procedure TabReinstallClick(Sender: TObject);
begin
  SwitchTab(TAB_REINSTALL);
end;

procedure TabUninstallClick(Sender: TObject);
begin
  SwitchTab(TAB_UNINSTALL);
end;

procedure TabBackupsClick(Sender: TObject);
begin
  SwitchTab(TAB_BACKUPS);
end;

procedure MinClick(Sender: TObject);
begin
  ShowWindow(WizardForm.Handle, SW_MINIMIZE);
end;

procedure CloseClick(Sender: TObject);
begin
  if ScreenState = SCREEN_PROGRESS then
    WizardForm.CancelButton.OnClick(nil)
  else
    WizardForm.Close;
end;

procedure CancelClick(Sender: TObject);
begin
  if ScreenState = SCREEN_PROGRESS then
    WizardForm.CancelButton.OnClick(nil)
  else
    WizardForm.Close;
end;

procedure InstallDirChanged(Sender: TObject);
begin
  ChosenInstallDir := CurrentInstallDir;
  if ScreenState = SCREEN_TABS then
  begin
    UpdateHeader;
    UpdateInstallPanel;
  end;
end;

procedure DesktopChanged(Sender: TObject);
begin
  DesktopWanted := DesktopCheck.Checked;
  if DesktopCheck.Checked then
    SetStatus('Skrót na pulpicie zostanie utworzony.', 0)
  else
    SetStatus('Skrót na pulpicie nie zostanie utworzony.', 0);
end;

procedure BrowseClick(Sender: TObject);
var
  Dir: String;
begin
  Dir := CurrentInstallDir;
  if BrowseForFolder('Wybierz folder instalacji KD Core', Dir, True) then
  begin
    Dir := NormalizeDir(Dir);
    if Dir = '' then
      Exit;
    InstallDirEdit.Text := Dir;
    ChosenInstallDir := Dir;
    UpdateHeader;
    UpdateInstallPanel;
  end;
end;

procedure SelectRepair(Sender: TObject);
begin
  CleanOptionSelected := False;
  SetButtonPng(RepairButton, 'opt_repair_sel.png');
  SetButtonPng(CleanButton, 'opt_clean.png');
  SetStatus('Naprawa zachowa ustawienia KD Core.', 0);
end;

procedure SelectClean(Sender: TObject);
begin
  CleanOptionSelected := True;
  SetButtonPng(RepairButton, 'opt_repair.png');
  SetButtonPng(CleanButton, 'opt_clean_sel.png');
  SetStatus('Czysta instalacja usunie ustawienia KD Core.', 3);
end;

procedure CreateTopBar;
begin
  TopBar := TPanel.Create(Shell);
  TopBar.Parent := Shell;
  TopBar.SetBounds(0, 0, ScaleX(560), ScaleY(44));
  TopBar.BevelOuter := bvNone;
  TopBar.Color := ClrSecondary;

  TopBorder := TPanel.Create(Shell);
  TopBorder.Parent := Shell;
  TopBorder.SetBounds(0, ScaleY(44), ScaleX(560), ScaleY(1));
  TopBorder.BevelOuter := bvNone;
  TopBorder.Color := ClrBorder;

  Logo := NewCard(TopBar, 16, 10, 24, 24, 'ui_logo.png');
  Logo.Stretch := True;

  NameLabel := NewLabel(TopBar, 'KD Core', 48, 9, 100, 24, 11, 'Segoe UI Semibold', ClrText);
  VersionLabel := NewLabel(TopBar, 'v' + AppVersion, 148, 12, 90, 20, 8, 'Segoe UI', ClrMuted);

  MinLabel := NewLabel(TopBar, '–', 488, 0, 32, 43, 12, 'Segoe UI', ClrMuted);
  MinLabel.Alignment := taCenter;
  MinLabel.Transparent := False;
  MinLabel.Color := ClrSecondary;
  MinLabel.Cursor := crHand;
  MinLabel.OnClick := @MinClick;

  CloseLabel := NewLabel(TopBar, '×', 520, 0, 40, 43, 14, 'Segoe UI', ClrMuted);
  CloseLabel.Alignment := taCenter;
  CloseLabel.Transparent := False;
  CloseLabel.Color := ClrSecondary;
  CloseLabel.Cursor := crHand;
  CloseLabel.OnClick := @CloseClick;
end;

procedure CreateHeader;
begin
  TitleLabel := NewLabel(Shell, 'Zainstaluj KD Core', 28, 64, 504, 38, 20, 'Segoe UI Semibold', ClrText);
  DescLabel := NewLabel(Shell, '', 28, 102, 504, 22, 10, 'Segoe UI', ClrMuted);
  MetaLabel := NewLabel(Shell, '', 28, 126, 504, 20, 8, 'Segoe UI', ClrMuted);
end;

procedure CreateTabs;
begin
  TabLabels[0] := NewLabel(Shell, 'Instaluj', 28, 148, 104, 30, 10, 'Segoe UI Semibold', ClrMuted);
  TabLabels[1] := NewLabel(Shell, 'Reinstaluj', 140, 148, 104, 30, 10, 'Segoe UI Semibold', ClrMuted);
  TabLabels[2] := NewLabel(Shell, 'Odinstaluj', 252, 148, 104, 30, 10, 'Segoe UI Semibold', ClrMuted);
  TabLabels[3] := NewLabel(Shell, 'Kopie zapasowe', 364, 148, 112, 30, 10, 'Segoe UI Semibold', ClrMuted);
  TabLabels[0].OnClick := @TabInstallClick;
  TabLabels[1].OnClick := @TabReinstallClick;
  TabLabels[2].OnClick := @TabUninstallClick;
  TabLabels[3].OnClick := @TabBackupsClick;

  TabDivider := NewLabel(Shell, '', 28, 180, 504, 1, 8, 'Segoe UI', ClrBorder);
  TabDivider.Transparent := False;
  TabDivider.Color := ClrBorder;

  TabUnderline := NewCard(Shell, 28, 176, 104, 8, 'ui_tab_indicator.png');
end;

procedure CreateInstallPanel;
begin
  InstallPanel := TPanel.Create(Shell);
  InstallPanel.Parent := Shell;
  InstallPanel.SetBounds(ScaleX(24), ScaleY(196), ScaleX(512), ScaleY(292));
  InstallPanel.BevelOuter := bvNone;
  InstallPanel.Color := ClrBg;

  InstallCard := NewCard(InstallPanel, 0, 0, 512, 104, 'ui_card.png');
  InstallSysCaption := NewLabel(InstallPanel, 'System', 20, 16, 140, 18, 9, 'Segoe UI', ClrMuted);
  InstallSysValue := NewLabel(InstallPanel, '', 168, 16, 328, 18, 9, 'Segoe UI', ClrText);
  InstallFreeCaption := NewLabel(InstallPanel, 'Wolne miejsce', 20, 44, 140, 18, 9, 'Segoe UI', ClrMuted);
  InstallFreeValue := NewLabel(InstallPanel, '', 168, 44, 328, 18, 9, 'Segoe UI', ClrText);
  InstallVerCaption := NewLabel(InstallPanel, 'KD Core', 20, 72, 140, 18, 9, 'Segoe UI', ClrMuted);
  InstallVerValue := NewLabel(InstallPanel, '', 168, 72, 328, 18, 9, 'Segoe UI', ClrText);

  InstallPathLabel := NewLabel(InstallPanel, 'Folder instalacji', 0, 124, 512, 20, 10, 'Segoe UI Semibold', ClrText);
  InstallField := NewCard(InstallPanel, 0, 146, 512, 44, 'ui_field.png');
  InstallDirEdit := NewEdit(InstallPanel, 16, 157, 388, 22);
  InstallDirEdit.OnChange := @InstallDirChanged;
  BrowseButton := NewButton(InstallPanel, 416, 152, 84, 32, 'btn_browse.png');
  BrowseButton.OnClick := @BrowseClick;

  DesktopCheck := NewCheck(InstallPanel, 0, 202, 512, 24, 'Utwórz skrót na pulpicie', True);
  DesktopCheck.OnClick := @DesktopChanged;

  InstallNoteLabel := NewWrappedLabel(InstallPanel, '', 0, 238, 512, 44, 9, 'Segoe UI', ClrMuted);
end;

procedure CreateReinstallPanel;
begin
  ReinstallPanel := TPanel.Create(Shell);
  ReinstallPanel.Parent := Shell;
  ReinstallPanel.SetBounds(ScaleX(24), ScaleY(196), ScaleX(512), ScaleY(292));
  ReinstallPanel.BevelOuter := bvNone;
  ReinstallPanel.Color := ClrBg;

  ReinstallCard := NewCard(ReinstallPanel, 0, 0, 512, 104, 'ui_card.png');
  ReVersionCaption := NewLabel(ReinstallPanel, 'Wersja', 20, 16, 140, 18, 9, 'Segoe UI', ClrMuted);
  ReVersionValue := NewLabel(ReinstallPanel, '', 168, 16, 328, 18, 9, 'Segoe UI', ClrText);
  ReFolderCaption := NewLabel(ReinstallPanel, 'Folder', 20, 44, 140, 18, 9, 'Segoe UI', ClrMuted);
  ReFolderValue := NewLabel(ReinstallPanel, '', 168, 44, 328, 18, 8, 'Segoe UI', ClrText);
  ReFilesCaption := NewLabel(ReinstallPanel, 'Pliki programu', 20, 72, 140, 18, 9, 'Segoe UI', ClrMuted);
  ReFilesValue := NewLabel(ReinstallPanel, '', 168, 72, 328, 18, 9, 'Segoe UI', ClrText);

  ReOptionsLabel := NewLabel(ReinstallPanel, 'Tryb działania', 0, 124, 512, 20, 10, 'Segoe UI Semibold', ClrText);
  RepairButton := NewButton(ReinstallPanel, 0, 146, 512, 48, 'opt_repair_sel.png');
  RepairButton.OnClick := @SelectRepair;
  CleanButton := NewButton(ReinstallPanel, 0, 200, 512, 48, 'opt_clean.png');
  CleanButton.OnClick := @SelectClean;
  ReinstallNoteLabel := NewWrappedLabel(ReinstallPanel, '', 0, 256, 512, 34, 8, 'Segoe UI', ClrMuted);
end;

procedure CreateUninstallPanel;
begin
  UninstallPanel := TPanel.Create(Shell);
  UninstallPanel.Parent := Shell;
  UninstallPanel.SetBounds(ScaleX(24), ScaleY(196), ScaleX(512), ScaleY(292));
  UninstallPanel.BevelOuter := bvNone;
  UninstallPanel.Color := ClrBg;

  UninstallCard := NewCard(UninstallPanel, 0, 0, 512, 104, 'ui_card.png');
  UnVersionCaption := NewLabel(UninstallPanel, 'Wersja', 20, 16, 140, 18, 9, 'Segoe UI', ClrMuted);
  UnVersionValue := NewLabel(UninstallPanel, '', 168, 16, 328, 18, 9, 'Segoe UI', ClrText);
  UnFolderCaption := NewLabel(UninstallPanel, 'Folder', 20, 44, 140, 18, 9, 'Segoe UI', ClrMuted);
  UnFolderValue := NewLabel(UninstallPanel, '', 168, 44, 328, 18, 8, 'Segoe UI', ClrText);
  UnFilesCaption := NewLabel(UninstallPanel, 'Pliki programu', 20, 72, 140, 18, 9, 'Segoe UI', ClrMuted);
  UnFilesValue := NewLabel(UninstallPanel, '', 168, 72, 328, 18, 9, 'Segoe UI', ClrText);

  KeepSettingsCheck := NewCheck(UninstallPanel, 0, 124, 512, 28, 'Zachowaj ustawienia KD Core (zalecane)', True);
  UninstallNoteLabel := NewWrappedLabel(UninstallPanel, '', 0, 168, 512, 56, 9, 'Segoe UI', ClrMuted);
end;

procedure CreateBackupsPanel;
begin
  BackupsPanel := TPanel.Create(Shell);
  BackupsPanel.Parent := Shell;
  BackupsPanel.SetBounds(ScaleX(24), ScaleY(196), ScaleX(512), ScaleY(292));
  BackupsPanel.BevelOuter := bvNone;
  BackupsPanel.Color := ClrBg;

  BackupIntroLabel := NewLabel(BackupsPanel, 'Kopie zapasowe ustawień KD Core', 0, 0, 512, 20, 10, 'Segoe UI Semibold', ClrText);
  BackupPathLabel := NewLabel(BackupsPanel, '', 0, 24, 512, 20, 8, 'Segoe UI', ClrMuted);

  BackupCard := NewCard(BackupsPanel, 0, 50, 512, 212, 'ui_card_list.png');
  BackupList := TNewListBox.Create(BackupsPanel);
  BackupList.Parent := BackupsPanel;
  BackupList.SetBounds(ScaleX(16), ScaleY(62), ScaleX(480), ScaleY(148));
  BackupList.BorderStyle := bsNone;
  BackupList.Color := ClrField;
  BackupList.Font.Name := 'Segoe UI';
  BackupList.Font.Size := 9;
  BackupList.Font.Color := ClrText;

  BackupEmptyLabel := NewLabel(BackupsPanel, 'Brak kopii zapasowych. Utwórz pierwszą kopię.', 32, 110, 448, 40, 9, 'Segoe UI', ClrMuted);
  BackupEmptyLabel.Alignment := taCenter;

  RestoreButton := NewButton(BackupsPanel, 16, 220, 150, 36, 'btn_restore.png');
  RestoreButton.OnClick := @RestoreClick;
  RefreshLabel := NewLabel(BackupsPanel, 'Odśwież listę', 316, 220, 180, 36, 9, 'Segoe UI Semibold', ClrAccent);
  RefreshLabel.Alignment := taRightJustify;
  RefreshLabel.Cursor := crHand;
  RefreshLabel.OnClick := @RefreshClick;

  BackupNoteLabel := NewLabel(BackupsPanel, 'Kopia zawiera ustawienia KD Core. Raporty nie są zmieniane.', 0, 272, 512, 20, 8, 'Segoe UI', ClrMuted);
end;

procedure CreateProgressPanel;
begin
  ProgressPanel := TPanel.Create(Shell);
  ProgressPanel.Parent := Shell;
  ProgressPanel.SetBounds(ScaleX(24), ScaleY(196), ScaleX(512), ScaleY(292));
  ProgressPanel.BevelOuter := bvNone;
  ProgressPanel.Color := ClrBg;

  ProgressTitleLabel := NewLabel(ProgressPanel, 'Szczegóły operacji', 0, 0, 512, 20, 10, 'Segoe UI Semibold', ClrText);
  ProgressDescLabel := NewWrappedLabel(ProgressPanel, '', 0, 24, 512, 20, 9, 'Segoe UI', ClrMuted);
  ProgressDescLabel.Visible := False;

  ProgressCard := NewCard(ProgressPanel, 0, 28, 512, 104, 'ui_card.png');
  ProgOpCaption := NewLabel(ProgressPanel, 'Działanie', 20, 54, 120, 18, 9, 'Segoe UI', ClrMuted);
  ProgOpValue := NewLabel(ProgressPanel, '', 148, 54, 348, 18, 9, 'Segoe UI', ClrText);
  ProgPathCaption := NewLabel(ProgressPanel, 'Folder', 20, 88, 120, 18, 9, 'Segoe UI', ClrMuted);
  ProgPathValue := NewLabel(ProgressPanel, '', 148, 88, 348, 18, 8, 'Segoe UI', ClrText);

  ProgressHintLabel := NewWrappedLabel(ProgressPanel, 'Nie zamykaj okna instalatora. Po zakończeniu zobaczysz podsumowanie.', 0, 152, 512, 40, 9, 'Segoe UI', ClrMuted);
end;

procedure CreateFinishPanel;
begin
  FinishPanel := TPanel.Create(Shell);
  FinishPanel.Parent := Shell;
  FinishPanel.SetBounds(ScaleX(24), ScaleY(196), ScaleX(512), ScaleY(292));
  FinishPanel.BevelOuter := bvNone;
  FinishPanel.Color := ClrBg;

  FinishIcon := NewCard(FinishPanel, 0, 8, 32, 32, 'ui_check.png');
  FinishHeadingLabel := NewLabel(FinishPanel, '', 44, 6, 468, 36, 16, 'Segoe UI Semibold', ClrText);
  FinishDetailLabel := NewWrappedLabel(FinishPanel, '', 0, 56, 512, 120, 9, 'Segoe UI', ClrMuted);
  RunCheck := NewCheck(FinishPanel, 0, 196, 400, 24, 'Uruchom KD Core po zakończeniu', True);
end;

procedure CreateFooter;
begin
  FooterBorder := TPanel.Create(Shell);
  FooterBorder.Parent := Shell;
  FooterBorder.SetBounds(0, ScaleY(500), ScaleX(560), ScaleY(1));
  FooterBorder.BevelOuter := bvNone;
  FooterBorder.Color := ClrBorder;

  StatusLabel := NewLabel(Shell, '', 28, 508, 360, 18, 9, 'Segoe UI', ClrMuted);
  PercentLabel := NewLabel(Shell, '', 396, 508, 136, 18, 9, 'Segoe UI Semibold', ClrAccent);
  PercentLabel.Alignment := taRightJustify;

  ProgressTrack := TPanel.Create(Shell);
  ProgressTrack.Parent := Shell;
  ProgressTrack.SetBounds(ScaleX(28), ScaleY(534), ScaleX(504), ScaleY(4));
  ProgressTrack.BevelOuter := bvNone;
  ProgressTrack.Color := ClrTrack;

  ProgressFill := TPanel.Create(ProgressTrack);
  ProgressFill.Parent := ProgressTrack;
  ProgressFill.SetBounds(0, 0, 0, ScaleY(4));
  ProgressFill.BevelOuter := bvNone;
  ProgressFill.Color := ClrAccent;

  CancelLabel := NewLabel(Shell, 'Anuluj', 28, 552, 120, 40, 9, 'Segoe UI', ClrMuted);
  CancelLabel.Cursor := crHand;
  CancelLabel.OnClick := @CancelClick;

  PrimaryButton := NewButton(Shell, 312, 552, 224, 40, 'btn_install.png');
  PrimaryButton.OnClick := @PrimaryClick;
end;

procedure CreateShell;
begin
  Shell := TPanel.Create(WizardForm);
  Shell.Parent := WizardForm;
  Shell.Align := alClient;
  Shell.BevelOuter := bvNone;
  Shell.Color := ClrBg;

  BGImage := NewCard(Shell, 0, 0, 560, 620, 'ui_bg.png');

  CreateTopBar;
  CreateHeader;
  CreateTabs;
  CreateInstallPanel;
  CreateReinstallPanel;
  CreateUninstallPanel;
  CreateBackupsPanel;
  CreateProgressPanel;
  CreateFinishPanel;
  CreateFooter;


  ShellReady := True;
  ActiveTab := TAB_INSTALL;
  SetScreenState(SCREEN_TABS);
  TabUnderline.Left := ScaleX(28);
  SetStatus('Gotowe. Wybierz działanie.', 0);
  RefreshHover;
end;

function ValidateInstallDir(const Dir: String; var Msg: String): Boolean;
var
  D, WinDir, Drive: String;
begin
  Result := False;
  D := NormalizeDir(Dir);
  if D = '' then
  begin
    Msg := 'Podaj folder instalacji KD Core.';
    Exit;
  end;
  if IsDriveRoot(D) then
  begin
    Msg := 'Nie można zainstalować programu bezpośrednio w katalogu głównym dysku.';
    Exit;
  end;
  Drive := ExtractFileDrive(D);
  if (Drive = '') or (not DirExists(AddBackslash(Drive))) then
  begin
    Msg := 'Wybrany dysk nie istnieje.';
    Exit;
  end;
  if FileExists(D) then
  begin
    Msg := 'Wybrana ścieżka wskazuje plik, a nie folder.';
    Exit;
  end;
  WinDir := NormalizeDir(ExpandConstant('{win}'));
  if (WinDir <> '') and (Pos(Lowercase(WinDir), Lowercase(D)) = 1) then
  begin
    Msg := 'Nie można instalować programu w folderze systemowym Windows.';
    Exit;
  end;
  Result := True;
end;

procedure SetImagePng(Img: TBitmapImage; const AFileName: String);
var
  Path: String;
begin
  if Img = nil then
    Exit;
  Path := ExpandConstant('{tmp}\') + AFileName;
  if FileExists(Path) then
  try
    Img.PngImage.LoadFromFile(Path);
    Img.Invalidate;
  except
  end;
end;

procedure RefreshBackupList;
var
  FindRec: TFindRec;
  TempList: TStringList;
  I: Integer;
begin
  if BackupList = nil then
    Exit;
  BackupList.Items.Clear;
  BackupEntries.Clear;
  if ForceDirectories(BackupDir) then
  begin
    if FindFirst(AddBackslash(BackupDir) + 'settings_*.reg', FindRec) then
    begin
      try
        repeat
          if (FindRec.Attributes and FILE_ATTRIBUTE_DIRECTORY) = 0 then
            BackupEntries.Add(FindRec.Name);
        until not FindNext(FindRec);
      finally
        FindClose(FindRec);
      end;
    end;
  end;

  BackupEntries.Sort;
  TempList := TStringList.Create;
  try
    for I := BackupEntries.Count - 1 downto 0 do
      TempList.Add(BackupEntries[I]);
    BackupEntries.Assign(TempList);
  finally
    TempList.Free;
  end;

  for I := 0 to BackupEntries.Count - 1 do
    BackupList.Items.Add(FormatBackupBase(BackupEntries[I]));

  BackupEmptyLabel.Visible := BackupEntries.Count = 0;
  BackupList.Visible := BackupEntries.Count > 0;
  if BackupEntries.Count > 0 then
    BackupList.ItemIndex := 0;
end;

function CreateBackup: Boolean;
var
  Stamp, SettingsFile, LegacyFile, LegacyName, Params: String;
  Code: Integer;
begin
  Result := False;
  if not ForceDirectories(BackupDir) then
  begin
    SetStatus('Nie można utworzyć folderu kopii zapasowych.', 2);
    MsgBox('Nie można utworzyć folderu:' + #13#10 + BackupDir, mbError, MB_OK);
    Exit;
  end;
  if not RegKeyExists(HKEY_CURRENT_USER, SettingsKey) then
  begin
    SetStatus('Brak ustawień KD Core do zapisania.', 2);
    MsgBox('Nie znaleziono ustawień KD Core. Uruchom program przynajmniej raz, aby utworzyć ustawienia.', mbInformation, MB_OK);
    Exit;
  end;

  Stamp := GetDateTimeString('yyyy-mm-dd_hh-nn-ss', '-', '-');
  SettingsFile := AddBackslash(BackupDir) + 'settings_' + Stamp + '.reg';
  Params := 'export "HKEY_CURRENT_USER\' + SettingsKey + '" "' + SettingsFile + '" /y';
  if (not Exec(ExpandConstant('{sys}\reg.exe'), Params, '', SW_HIDE, ewWaitUntilTerminated, Code)) or (Code <> 0) or (not FileExists(SettingsFile)) then
  begin
    SetStatus('Nie udało się utworzyć kopii zapasowej.', 2);
    MsgBox('Eksport ustawień nie powiódł się (kod ' + IntToStr(Code) + ').', mbError, MB_OK);
    Exit;
  end;

  if RegKeyExists(HKEY_CURRENT_USER, LegacySettingsKey) then
  begin
    LegacyName := 'legacy_' + Copy(SettingsFile, Length(BackupDir) + 11, Length(SettingsFile) - Length(BackupDir) - 10);
    LegacyFile := AddBackslash(BackupDir) + LegacyName;
    Params := 'export "HKEY_CURRENT_USER\' + LegacySettingsKey + '" "' + LegacyFile + '" /y';
    Exec(ExpandConstant('{sys}\reg.exe'), Params, '', SW_HIDE, ewWaitUntilTerminated, Code);
  end;

  RefreshBackupList;
  SetStatus('Kopia zapasowa została utworzona: ' + FormatBackupBase(ExtractFileName(SettingsFile)), 1);
  Result := True;
end;

procedure RestoreSelectedBackup;
var
  SettingsFile, LegacyFile, LegacyName, Params: String;
  Code: Integer;
begin
  if (BackupList = nil) or (BackupList.ItemIndex < 0) or (BackupList.ItemIndex >= BackupEntries.Count) then
  begin
    SetStatus('Wybierz kopię zapasową z listy.', 2);
    MsgBox('Najpierw wybierz kopię zapasową z listy.', mbInformation, MB_OK);
    Exit;
  end;

  SettingsFile := AddBackslash(BackupDir) + BackupEntries[BackupList.ItemIndex];
  if MsgBox('Przywrócić ustawienia z kopii:' + #13#10 + FormatBackupBase(ExtractFileName(SettingsFile)) + '?',
    mbConfirmation, MB_YESNO) <> IDYES then
    Exit;

  Params := 'import "' + SettingsFile + '"';
  if (not Exec(ExpandConstant('{sys}\reg.exe'), Params, '', SW_HIDE, ewWaitUntilTerminated, Code)) or (Code <> 0) then
  begin
    SetStatus('Nie udało się przywrócić ustawień.', 2);
    MsgBox('Import ustawień nie powiódł się (kod ' + IntToStr(Code) + ').', mbError, MB_OK);
    Exit;
  end;

  LegacyName := 'legacy_' + Copy(SettingsFile, Length(BackupDir) + 11, Length(SettingsFile) - Length(BackupDir) - 10);
  LegacyFile := AddBackslash(BackupDir) + LegacyName;
  if FileExists(LegacyFile) then
  begin
    Params := 'import "' + LegacyFile + '"';
    Exec(ExpandConstant('{sys}\reg.exe'), Params, '', SW_HIDE, ewWaitUntilTerminated, Code);
  end;

  DetectInstallation;
  RefreshTabContent;
  SetStatus('Przywrócono ustawienia KD Core.', 1);
end;

procedure RestoreClick(Sender: TObject);
begin
  RestoreSelectedBackup;
end;

procedure RefreshClick(Sender: TObject);
begin
  RefreshBackupList;
  SetStatus('Lista kopii zapasowych została odświeżona.', 0);
end;

procedure StartProgressScreen(const ATitle, ADesc, AOperation, APath: String);
begin
  SetScreenState(SCREEN_PROGRESS);
  TitleLabel.Caption := ATitle;
  DescLabel.Caption := ADesc;
  MetaLabel.Caption := BuildMeta(APath);
  ProgOpValue.Caption := AOperation;
  ProgPathValue.Caption := APath;
  PercentLabel.Caption := '0%';
  ProgressFill.Width := 0;
  HoverTarget := HOVER_NONE;
  PrimaryDown := False;
  RefreshHover;
end;

procedure StartInstall(AClean: Boolean);
var
  Err, Op: String;
begin
  ChosenInstallDir := CurrentInstallDir;
  if not ValidateInstallDir(ChosenInstallDir, Err) then
  begin
    SetStatus(Err, 2);
    MsgBox(Err, mbError, MB_OK);
    Exit;
  end;

  CleanInstall := AClean;
  if Installed then
    Mode := MODE_REINSTALL
  else
    Mode := MODE_INSTALL;

  if AClean then
    Op := 'Czysta instalacja'
  else if Mode = MODE_REINSTALL then
    Op := 'Naprawa plików'
  else
    Op := 'Instalacja';

  WizardForm.DirEdit.Text := ChosenInstallDir;
  StartProgressScreen('Instalowanie KD Core',
    'Trwa zapisywanie plików programu. Nie zamykaj okna.',
    Op, ChosenInstallDir);
  SetStatus('Przygotowywanie instalacji...', 0);
  WizardForm.NextButton.Enabled := True;
  WizardForm.NextButton.OnClick(WizardForm.NextButton);
end;

procedure StartUninstall;
var
  Uninstaller, Params, Script, Flags, Q: String;
  Code: Integer;
begin
  if not Installed then
  begin
    SetStatus('KD Core nie jest zainstalowany.', 2);
    MsgBox('KD Core nie jest zainstalowany na tym komputerze.', mbInformation, MB_OK);
    Exit;
  end;
  if not FindUninstaller(Uninstaller) then
  begin
    SetStatus('Nie znaleziono deinstalatora KD Core.', 2);
    MsgBox('Nie znaleziono pliku deinstalatora. Spróbuj najpierw naprawić instalację.', mbError, MB_OK);
    Exit;
  end;
  if MsgBox('Odinstalować KD Core z tego komputera?', mbConfirmation, MB_YESNO) <> IDYES then
    Exit;

  KeepSettingsWanted := KeepSettingsCheck.Checked;
  Mode := MODE_UNINSTALL;
  UninstallMarker := ExpandConstant('{tmp}\kd_uninstall_done.txt');
  UninstallCmdFile := ExpandConstant('{tmp}\kd_uninstall.cmd');
  DeleteFile(UninstallMarker);

  Q := '"' + Uninstaller + '"';
  Flags := ' /VERYSILENT /SUPPRESSMSGBOXES /NORESTART';
  if not KeepSettingsWanted then
    Flags := Flags + ' /REMOVESETTINGS';

  Script := '@echo off' + #13#10 +
    'del /f /q "' + UninstallMarker + '" >nul 2>&1' + #13#10 +
    Q + Flags + #13#10 +
    'set RC=%ERRORLEVEL%' + #13#10 +
    'set /a N=0' + #13#10 +
    ':wait' + #13#10 +
    'tasklist /FO CSV /NH 2>nul | findstr /I /C:"unins" >nul' + #13#10 +
    'if errorlevel 1 goto done' + #13#10 +
    'set /a N+=1' + #13#10 +
    'if %N% GEQ 90 goto done' + #13#10 +
    'ping -n 2 127.0.0.1 >nul' + #13#10 +
    'goto wait' + #13#10 +
    ':done' + #13#10 +
    'echo %RC%>"' + UninstallMarker + '"' + #13#10;

  if not SaveStringToFile(UninstallCmdFile, Script, False) then
  begin
    SetStatus('Nie można przygotować deinstalacji.', 2);
    MsgBox('Nie można utworzyć pliku roboczego deinstalacji.', mbError, MB_OK);
    Exit;
  end;

  StartProgressScreen('Odinstalowywanie KD Core',
    'Trwa usuwanie programu. Nie zamykaj okna.',
    'Deinstalacja', InstalledDir);
  SetStatus('Zamykanie KD Core...', 0);

  UninstallStarted := True;
  UninstallTicks := 0;
  UninstallPercent := 5;
  PercentLabel.Caption := '5%';
  ProgressFill.Width := (ProgressTrack.Width * 5) div 100;

  Params := '/C ""' + UninstallCmdFile + '""';
  if not Exec(ExpandConstant('{cmd}'), Params, '', SW_HIDE, ewNoWait, Code) then
  begin
    UninstallStarted := False;
    SetScreenState(SCREEN_TABS);
    RefreshTabContent;
    SetStatus('Nie można uruchomić deinstalatora.', 2);
  end;
end;

procedure ShowFinishScreen;
begin
  SetScreenState(SCREEN_FINISH);
  TitleLabel.Caption := 'Gotowe';
  DescLabel.Caption := 'Operacja została zakończona.';
  MetaLabel.Caption := BuildMeta('');
  UpdateFinishPanel;
  PrimaryAsset := 'btn_finish';
  PrimaryVisualKey := '';
  SetBtnVisual(PrimaryButton, PrimaryAsset, False, False, PrimaryVisualKey);
  PrimaryButton.Visible := True;
  PercentLabel.Caption := '';
  ProgressTrack.Visible := False;
  HoverTarget := HOVER_NONE;
  PrimaryDown := False;
  RefreshHover;
end;

procedure CompleteUninstall;
var
  Raw: AnsiString;
begin
  UninstallStarted := False;
  LastExitCode := -1;
  if FileExists(UninstallMarker) then
  begin
    if LoadStringFromFile(UninstallMarker, Raw) then
      LastExitCode := StrToIntDef(Trim(String(Raw)), -1);
  end;

  DetectInstallation;
  if not Installed then
  begin
    PostInstallOk := True;
    SetStatus('KD Core został odinstalowany.', 1);
    ShowFinishScreen;
  end
  else
  begin
    SetScreenState(SCREEN_TABS);
    ActiveTab := TAB_UNINSTALL;
    RefreshTabContent;
    SetStatus('Deinstalacja nie została ukończona.', 2);
    MsgBox('Nie udało się usunąć KD Core. Sprawdź, czy program nie jest uruchomiony, i spróbuj ponownie.', mbError, MB_OK);
  end;
  DeleteFile(UninstallMarker);
  DeleteFile(UninstallCmdFile);
end;

procedure ProgressTimerProc(Arg1, Arg2, Arg3, Arg4: Longword);
var
  P, W: Integer;
begin
  if (ScreenState <> SCREEN_PROGRESS) or (WizardForm = nil) or (ProgressTrack = nil) then
    Exit;

  if UninstallStarted then
  begin
    Inc(UninstallTicks);
    if (UninstallTicks mod 2 = 0) and (UninstallPercent < 90) then
      Inc(UninstallPercent);
    PercentLabel.Caption := IntToStr(UninstallPercent) + '%';
    ProgressFill.Width := (ProgressTrack.Width * UninstallPercent) div 100;
    if FileExists(UninstallMarker) then
      CompleteUninstall;
    Exit;
  end;

  P := WizardForm.ProgressGauge.Position;
  if P < 0 then
    P := 0;
  if P > 100 then
    P := 100;
  W := (ProgressTrack.Width * P) div 100;
  if ProgressFill.Width <> W then
    ProgressFill.Width := W;
  PercentLabel.Caption := IntToStr(P) + '%';
  if WizardForm.StatusLabel.Caption <> '' then
    SetStatus(WizardForm.StatusLabel.Caption, 0);
end;

procedure UpdateFinishPanel;
var
  Icon, Heading, Detail, Folder: String;
begin
  if FinishPanel = nil then
    Exit;
  Folder := ExpandConstant('{app}');

  if Mode = MODE_UNINSTALL then
  begin
    Icon := 'ui_check.png';
    Heading := 'KD Core został odinstalowany';
    Detail := 'Program został usunięty z tego komputera.' + #13#10#13#10;
    if KeepSettingsWanted then
      Detail := Detail + 'Ustawienia KD Core pozostały zachowane.'
    else
      Detail := Detail + 'Ustawienia KD Core zostały usunięte.';
    Detail := Detail + #13#10 + 'Raporty w Dokumenty\KD Core\Raporty nie zostały zmienione.';
    RunCheck.Visible := False;
    SetStatus('Deinstalacja zakończona pomyślnie.', 1);
  end
  else if PostInstallOk then
  begin
    Icon := 'ui_check.png';
    if Mode = MODE_REINSTALL then
      Heading := 'KD Core został naprawiony'
    else
      Heading := 'KD Core został zainstalowany';
    Detail := 'Wersja ' + AppVersion + ' jest gotowa do użycia.' + #13#10#13#10 +
      'Folder: ' + Folder + #13#10;
    if DesktopWanted then
      Detail := Detail + 'Skrót na pulpicie: utworzony' + #13#10
    else
      Detail := Detail + 'Skrót na pulpicie: nie utworzono' + #13#10;
    Detail := Detail + 'Ustawienia i raporty w Dokumentach zostały zachowane.';
    RunCheck.Visible := FileExists(AddBackslash(Folder) + AppExe);
    RunCheck.Checked := RunCheck.Visible;
    SetStatus('Operacja zakończona pomyślnie.', 1);
  end
  else
  begin
    Icon := 'ui_warn.png';
    Heading := 'Instalacja zakończona z ostrzeżeniem';
    Detail := 'Pliki zostały zapisane, ale nie udało się potwierdzić ich integralności.' + #13#10#13#10 +
      'Uruchom instalator ponownie lub sprawdź raport w folderze Dokumenty.';
    RunCheck.Visible := False;
    SetStatus('Nie udało się potwierdzić integralności plików.', 2);
  end;

  SetImagePng(FinishIcon, Icon);
  FinishHeadingLabel.Caption := Heading;
  FinishDetailLabel.Caption := Detail;

  PrimaryAsset := 'btn_finish';
  PrimaryVisualKey := '';
  SetBtnVisual(PrimaryButton, PrimaryAsset, False, False, PrimaryVisualKey);
  PrimaryButton.Visible := True;
  PrimaryButton.Enabled := True;
end;

procedure FinishClose;
var
  ExePath: String;
  Code: Integer;
begin
  if (Mode <> MODE_UNINSTALL) and RunCheck.Visible and RunCheck.Checked then
  begin
    ExePath := ExpandConstant('{app}\' + AppExe);
    if FileExists(ExePath) then
      ShellExec('', ExePath, '', ExtractFileDir(ExePath), SW_SHOWNORMAL, ewNoWait, Code);
  end;
  WizardForm.Close;
end;

procedure DispatchPrimary;
var
  Err: String;
begin
  case ActiveTab of
    TAB_INSTALL:
      begin
        ChosenInstallDir := CurrentInstallDir;
        if not ValidateInstallDir(ChosenInstallDir, Err) then
        begin
          SetStatus(Err, 2);
          MsgBox(Err, mbError, MB_OK);
          Exit;
        end;
        if Installed then
          if MsgBox('KD Core jest już zainstalowany. Odświeżyć pliki programu, zachowując ustawienia?',
            mbConfirmation, MB_YESNO) <> IDYES then
            Exit;
        StartInstall(False);
      end;
    TAB_REINSTALL:
      begin
        if not Installed then
        begin
          SetStatus('KD Core nie jest zainstalowany.', 2);
          MsgBox('KD Core nie jest zainstalowany. Użyj zakładki Instaluj.', mbInformation, MB_OK);
          Exit;
        end;
        if CleanOptionSelected then
          if MsgBox('Czysta instalacja usunie ustawienia KD Core, ale pozostawi raporty. Kontynuować?',
            mbConfirmation, MB_YESNO) <> IDYES then
            Exit;
        StartInstall(CleanOptionSelected);
      end;
    TAB_UNINSTALL:
      StartUninstall;
    TAB_BACKUPS:
      CreateBackup;
  end;
end;

procedure PrimaryClick(Sender: TObject);
begin
  if ScreenState = SCREEN_FINISH then
    FinishClose
  else if ScreenState = SCREEN_TABS then
    DispatchPrimary;
end;

procedure CleanupUninstallFolder;
var
  Dir, CurrentBase, Name: String;
  FindRec: TFindRec;
begin
  Dir := ExpandConstant('{app}');
  if not DirExists(Dir) then
    Exit;

  DeleteFile(AddBackslash(Dir) + AppExe);
  DeleteFile(ExpandConstant('{autodesktop}\KD Core.lnk'));
  DeleteFile(ExpandConstant('{group}\KD Core.lnk'));

  CurrentBase := Lowercase(ChangeFileExt(ExtractFileName(ParamStr(0)), ''));
  if FindFirst(AddBackslash(Dir) + 'unins*.*', FindRec) then
  begin
    try
      repeat
        Name := Lowercase(FindRec.Name);
        if (FindRec.Attributes and FILE_ATTRIBUTE_DIRECTORY) = 0 then
          if Copy(Name, 1, Length(CurrentBase)) <> CurrentBase then
          begin
            DeleteFile(AddBackslash(Dir) + FindRec.Name);
            Log('Cleanup: removed leftover ' + FindRec.Name);
          end;
      until not FindNext(FindRec);
    finally
      FindClose(FindRec);
    end;
  end;

  RemoveDir(Dir);
end;

procedure ScheduleUninstallCleanup;
var
  Dir, Cmd: String;
  Code: Integer;
begin
  Dir := ExpandConstant('{app}');
  if not DirExists(Dir) then
    Exit;
  Cmd := '/C ping -n 3 127.0.0.1 >nul & del /f /q "' + AddBackslash(Dir) +
         'unins*.*" >nul 2>&1 & rmdir "' + Dir + '" >nul 2>&1';
  Exec(ExpandConstant('{cmd}'), Cmd, '', SW_HIDE, ewNoWait, Code);
end;

function InitializeSetup: Boolean;
begin
  CleanInstall := HasParam('/CLEAN');
  PostInstallOk := True;
  Mode := MODE_INSTALL;
  DetectInstallation;
  if Installed then
    Mode := MODE_REINSTALL;
  Log('Detected install=' + YesNo(Installed) + ' dir=' + InstalledDir + ' version=' + InstalledVersion);
  Result := True;
end;

{ Windows 11 rounds top-level windows through DWM with antialiased corners.
  Older builds get a DPI-scaled round-rectangle window region instead. }
procedure ApplyGlassWindowShape;
var
  CornerPref, Radius, Hr: Integer;
  Region: Longword;
begin
  if WizardForm = nil then
    Exit;
  CornerPref := 2; { DWMWA_WINDOW_CORNER_PREFERENCE = DWMWCP_ROUND }
  Hr := DwmSetWindowAttribute(WizardForm.Handle, 33, CornerPref, 4);
  if Hr = 0 then
    Exit;
  Radius := ScaleX(16);
  Region := CreateRoundRectRgn(0, 0, ScaleX(560) + 1, ScaleY(620) + 1, Radius, Radius);
  if Region <> 0 then
    SetWindowRgn(WizardForm.Handle, Region, True);
end;

procedure InitializeWizard;
begin
  if WizardSilent then
  begin
    DesktopWanted := WizardIsTaskSelected('desktopicon');
    Log('Silent install: desktop shortcut=' + YesNo(DesktopWanted));
    Exit;
  end;

  MainPage := CreateCustomPage(wpWelcome, ' ', ' ');

  WizardForm.BorderStyle := bsNone;
  WizardForm.Color := ClrBg;
  WizardForm.MainPanel.Visible := False;
  WizardForm.OuterNotebook.Visible := False;
  WizardForm.Bevel.Visible := False;
  WizardForm.Bevel1.Visible := False;
  WizardForm.NextButton.Visible := False;
  WizardForm.BackButton.Visible := False;
  WizardForm.CancelButton.Visible := False;
  WizardForm.ClientWidth := ScaleX(560);
  WizardForm.ClientHeight := ScaleY(620);
  WizardForm.SetBounds(WizardForm.Left, WizardForm.Top, ScaleX(560), ScaleY(620));
  WizardForm.Position := poScreenCenter;
  ApplyGlassWindowShape;

  ExtractTemporaryFiles('*.png');

  BackupDir := ExpandConstant('{userdocs}\KD Core\Kopie zapasowe');
  BackupEntries := TStringList.Create;
  ChosenInstallDir := ExpandConstant('{autopf}\KD Core');
  if Installed and (InstalledDir <> '') then
    ChosenInstallDir := InstalledDir;

  CreateShell;
  InstallDirEdit.Text := ChosenInstallDir;
  InstallDirEdit.SelStart := Length(InstallDirEdit.Text);
  InstallDirEdit.SelLength := 0;
  DesktopWanted := DesktopCheck.Checked;

  DetectInstallation;
  RefreshTabContent;
  if Installed then
    SetStatus('Wykryto KD Core ' + InstalledVersion + '.', 3)
  else
    SetStatus('Gotowe do instalacji.', 0);

  TimerID := SetTimer(0, 0, 60, CreateCallback(@ShellTimerProc));
  ProgressTimerID := SetTimer(0, 1, 100, CreateCallback(@ProgressTimerProc));
  WizardForm.ActiveControl := PrimaryButton;
end;

procedure DeinitializeSetup;
begin
  if TimerID <> 0 then
    KillTimer(0, TimerID);
  if ProgressTimerID <> 0 then
    KillTimer(0, ProgressTimerID);
  if BackupEntries <> nil then
    BackupEntries.Free;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then
  begin
    if HasParam('/CLEAN') then
      CleanInstall := True;

    DetectInstallation;
    if Installed then
      Mode := MODE_REINSTALL
    else
      Mode := MODE_INSTALL;
    Log('Install mode: ' + IntToStr(Mode));

    if CleanInstall then
    begin
      RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, SettingsKey);
      RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, LegacySettingsKey);
      Log('Clean install: user settings removed');
    end;

    if WizardSilent then
      DesktopWanted := WizardIsTaskSelected('desktopicon')
    else if DesktopCheck <> nil then
      DesktopWanted := DesktopCheck.Checked;
  end
  else if CurStep = ssPostInstall then
  begin
    if VerifyInstalledExe then
    begin
      PostInstallOk := True;
      Log('Post-install integrity check: OK');
    end
    else
    begin
      PostInstallOk := False;
      Log('Post-install integrity check: FAILED');
    end;

    if not DesktopWanted then
      DeleteFile(ExpandConstant('{autodesktop}\KD Core.lnk'));
  end;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := (PageID = wpWelcome) or (PageID = wpSelectTasks);
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if not WizardSilent then
  begin
    WizardForm.NextButton.Visible := False;
    WizardForm.BackButton.Visible := False;
    WizardForm.CancelButton.Visible := False;
  end;

  if CurPageID = wpInstalling then
  begin
    if WizardSilent then
      Exit;
    if not ShellReady then
      Exit;
    if ScreenState <> SCREEN_PROGRESS then
      StartProgressScreen('Instalowanie KD Core',
        'Trwa zapisywanie plików programu. Nie zamykaj okna.',
        'Instalacja', ChosenInstallDir);
    SetStatus('Instalowanie plików...', 0);
  end
  else if CurPageID = wpFinished then
  begin
    if WizardSilent then
      Exit;
    if not ShellReady then
      Exit;
    DetectInstallation;
    ShowFinishScreen;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Code: Integer;
begin
  if CurUninstallStep = usUninstall then
  begin
    Exec(ExpandConstant('{sys}\taskkill.exe'), '/F /IM ' + AppExe, '', SW_HIDE, ewWaitUntilTerminated, Code);
  end
  else if CurUninstallStep = usPostUninstall then
  begin
    if HasParam('/REMOVESETTINGS') then
    begin
      RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, SettingsKey);
      RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, LegacySettingsKey);
      Log('Remove-settings: done');
    end
    else
      Log('Remove-settings: not requested, user settings preserved');

    CleanupUninstallFolder;
  end;
end;

procedure DeinitializeUninstall;
begin
  CleanupUninstallFolder;
  ScheduleUninstallCleanup;
end;
