unit Main;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, ComCtrls, Menus,
  ExtCtrls, StdCtrls, ValEdit, Grids, TAGraph, Types
  {$IFDEF WINDOWS}, Windows, Registry{$ENDIF};

type

  // subset of the Delphi TServerInfo, only the fields lvServers shows
  TServerEntry = record
    Address: string;
    Port: Word;
    HostName: string;
    Players: Integer;
    MaxPlayers: Integer;
    Ping: Integer;
    Mode: string;
    Language: string;
    Passworded: Boolean;
  end;

  { TfmMain }

  TfmMain = class(TForm)
    cbFilterEmpty: TCheckBox;
    cbFilterPassworded: TCheckBox;
    cbFilterFull: TCheckBox;
    chSIPingChart: TChart;
    edSIPing: TLabeledEdit;
    edSIMap: TLabeledEdit;
    edSIMode: TLabeledEdit;
    edSIPlayers: TLabeledEdit;
    gbFilter: TGroupBox;
    gbInfo: TGroupBox;
    ilMain: TImageList;
    imLogo: TImage;
    edFilterMode: TLabeledEdit;
    edFilterMap: TLabeledEdit;
    edSIAddress: TLabeledEdit;
    lebName: TLabeledEdit;
    lvServers: TListView;
    lvPlayers: TListView;
    lvRules: TListView;
    N5: TMenuItem;
    miFile: TMenuItem;
    miQuickConnect: TMenuItem;
    miAddServer: TMenuItem;
    miDeleteServer: TMenuItem;
    miRefreshServer: TMenuItem;
    miMasterServerUpdate: TMenuItem;
    miCopyServerInfo: TMenuItem;
    miServerProperties: TMenuItem;
    miTools: TMenuItem;
    miSettings: TMenuItem;
    miHelp: TMenuItem;
    miAbout: TMenuItem;
    N4: TMenuItem;
    N3: TMenuItem;
    N2: TMenuItem;
    miImportFavoritesList: TMenuItem;
    miExportFavoritesList: TMenuItem;
    miExit: TMenuItem;
    miView: TMenuItem;
    miFilterServerInfo: TMenuItem;
    miStatusBar: TMenuItem;
    miServers: TMenuItem;
    miConnect: TMenuItem;
    N1: TMenuItem;
    mmMain: TMainMenu;
    Panel1: TPanel;
    pURL: TPanel;
    pnLine: TPanel;
    pnBreakable: TPanel;
    //pnLine: TPanel;
    sbMain: TStatusBar;
    Splitter1: TSplitter;
    Splitter2: TSplitter;
    tsServerLists: TTabControl;
    tbMain: TToolBar;
    tbConnect: TToolButton;
    tbCopyServerInfo: TToolButton;
    tbServerProperties: TToolButton;
    tbSpacer4: TToolButton;
    tbSettings: TToolButton;
    tbQuickConnect: TToolButton;
    tbSpacer1: TToolButton;
    tbAddServer: TToolButton;
    tbDeleteServer: TToolButton;
    tbRefreshServer: TToolButton;
    tbSpacer2: TToolButton;
    tbMasterServerUpdate: TToolButton;
    tbSpacer3: TToolButton;
    tbSpacer5: TToolButton;
    tbHelp: TToolButton;
    tbAbout: TToolButton;
    procedure AboutClick(Sender: TObject);
    procedure AddServerClick(Sender: TObject);
    procedure ConnectClick(Sender: TObject);
    procedure CopyServerInfoClick(Sender: TObject);
    procedure DeleteServerClick(Sender: TObject);
    procedure edFilterModeClick(Sender: TObject);
    procedure ExitClick(Sender: TObject);
    procedure ExportFavoritesClick(Sender: TObject);
    procedure FilterChange(Sender: TObject);
    procedure GroupBox1Click(Sender: TObject);
    procedure imLogoClick(Sender: TObject);
    procedure ImportFavoritesClick(Sender: TObject);
    procedure lbServersContextPopup(Sender: TObject; MousePos: TPoint;
      var Handled: Boolean);
    procedure MasterServerUpdateClick(Sender: TObject);
    procedure miViewClick(Sender: TObject);
    procedure Panel1Click(Sender: TObject);
    procedure pnBreakableResize(Sender: TObject);
    procedure QuickConnectClick(Sender: TObject);
    procedure RefreshServerClick(Sender: TObject);
    procedure ServerPropertiesClick(Sender: TObject);
    procedure SettingsClick(Sender: TObject);
    procedure Splitter1CanOffset(Sender: TObject; var NewOffset: Integer;
      var Accept: Boolean);
    procedure tbMainClick(Sender: TObject);
    procedure tbMainResize(Sender: TObject);
    procedure ToggleFilterServerInfo(Sender: TObject);
    procedure ToggleStatusBar(Sender: TObject);
    procedure tsServerListsChange(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure lvServersSelectItem(Sender: TObject; Item: TListItem;
      Selected: Boolean);
  private
    // favourites, held in memory only, nothing queries or persists them yet
    Servers: array of TServerEntry;
    function FindServer(const AAddress: string; APort: Word): Integer;
    procedure UpdateServerList;
    procedure UpdateSelectionUI;
    function SelectedServer: Integer;
    function GetGtaExePath: string;
    procedure ServerConnect(const AHost: string; APort: Word;
      const APassword: string);
  public

  end;

var
  fmMain: TfmMain;

implementation

{$R *.lfm}

// HOST:PORT, bare host falls back to 7777 the way the Delphi version does
procedure SplitHostPort(const AText: string; out AHost: string; out APort: Word);
var
  iColon: Integer;
begin
  AHost := Trim(AText);
  APort := 7777;

  iColon := Pos(':', AHost);
  if iColon > 0 then
  begin
    APort := Word(StrToIntDef(Trim(Copy(AHost, iColon + 1, Length(AHost) - iColon)), 7777));
    AHost := Trim(Copy(AHost, 1, iColon - 1));
  end;

  if APort = 0 then
    APort := 7777;
end;

{ TfmMain }

function TfmMain.FindServer(const AAddress: string; APort: Word): Integer;
var
  i: Integer;
begin
  for i := 0 to High(Servers) do
    if SameText(Servers[i].Address, AAddress) and (Servers[i].Port = APort) then
      Exit(i);
  Result := -1;
end;

function TfmMain.SelectedServer: Integer;
begin
  Result := lvServers.ItemIndex;
  if (Result < 0) or (Result > High(Servers)) then
    Result := -1;
end;

// mirrors lbServersClick in the Delphi version, everything that acts on a server
// is dead until one is selected
procedure TfmMain.UpdateSelectionUI;
var
  iSel: Integer;
  bHas: Boolean;
begin
  iSel := SelectedServer;
  bHas := iSel >= 0;

  tbDeleteServer.Enabled := bHas;
  miDeleteServer.Enabled := bHas;
  tbConnect.Enabled := bHas;
  miConnect.Enabled := bHas;
  tbRefreshServer.Enabled := bHas;
  miRefreshServer.Enabled := bHas;
  tbCopyServerInfo.Enabled := bHas;
  miCopyServerInfo.Enabled := bHas;
  tbServerProperties.Enabled := bHas;
  miServerProperties.Enabled := bHas;

  lvPlayers.Items.Clear;
  lvRules.Items.Clear;

  if not bHas then
  begin
    edSIAddress.Text := '- - -';
    edSIPlayers.Text := '- - -';
    edSIPing.Text := '- - -';
    edSIMode.Text := '- - -';
    edSIMap.Text := '- - -';
    gbInfo.Caption := ' Server Info ';
    Exit;
  end;

  edSIAddress.Text := Format('%s:%d', [Servers[iSel].Address, Servers[iSel].Port]);
  if Servers[iSel].MaxPlayers > 0 then
    edSIPlayers.Text := Format('%d/%d', [Servers[iSel].Players, Servers[iSel].MaxPlayers])
  else
    edSIPlayers.Text := '- - -';
  if Servers[iSel].Ping > 0 then
    edSIPing.Text := IntToStr(Servers[iSel].Ping)
  else
    edSIPing.Text := '- - -';
  if Servers[iSel].Mode <> '' then
    edSIMode.Text := Servers[iSel].Mode
  else
    edSIMode.Text := '- - -';
  edSIMap.Text := '- - -';
  gbInfo.Caption := ' ' + Servers[iSel].HostName + ' ';
end;

procedure TfmMain.lvServersSelectItem(Sender: TObject; Item: TListItem;
  Selected: Boolean);
begin
  UpdateSelectionUI;
end;

procedure TfmMain.FormCreate(Sender: TObject);
begin
  UpdateSelectionUI;
end;

procedure TfmMain.UpdateServerList;
var
  i: Integer;
  Item: TListItem;
begin
  lvServers.BeginUpdate;
  try
    lvServers.Items.Clear;
    for i := 0 to High(Servers) do
    begin
      Item := lvServers.Items.Add;
      // column 0 is the 28px lock column, the rest are subitems
      Item.Caption := '';
      Item.SubItems.Add(Servers[i].HostName);
      Item.SubItems.Add(Format('%d/%d', [Servers[i].Players, Servers[i].MaxPlayers]));
      if Servers[i].Ping > 0 then
        Item.SubItems.Add(IntToStr(Servers[i].Ping))
      else
        Item.SubItems.Add('');
      Item.SubItems.Add(Servers[i].Mode);
      Item.SubItems.Add(Servers[i].Language);
    end;
  finally
    lvServers.EndUpdate;
  end;
end;

procedure TfmMain.ImportFavoritesClick(Sender: TObject);
begin

end;

procedure TfmMain.lbServersContextPopup(Sender: TObject; MousePos: TPoint;
  var Handled: Boolean);
begin

end;

procedure TfmMain.MasterServerUpdateClick(Sender: TObject);
begin

end;

procedure TfmMain.miViewClick(Sender: TObject);
begin

end;

procedure TfmMain.Panel1Click(Sender: TObject);
begin

end;

procedure TfmMain.pnBreakableResize(Sender: TObject);
begin

end;

procedure TfmMain.QuickConnectClick(Sender: TObject);
begin

end;

procedure TfmMain.RefreshServerClick(Sender: TObject);
begin

end;

procedure TfmMain.ServerPropertiesClick(Sender: TObject);
begin

end;

procedure TfmMain.SettingsClick(Sender: TObject);
begin

end;

procedure TfmMain.Splitter1CanOffset(Sender: TObject; var NewOffset: Integer;
  var Accept: Boolean);
begin

end;

procedure TfmMain.tbMainClick(Sender: TObject);
begin

end;

procedure TfmMain.tbMainResize(Sender: TObject);
begin

end;

procedure TfmMain.ToggleFilterServerInfo(Sender: TObject);
begin

end;

procedure TfmMain.ToggleStatusBar(Sender: TObject);
begin

end;

procedure TfmMain.tsServerListsChange(Sender: TObject);
begin

end;

procedure TfmMain.ExportFavoritesClick(Sender: TObject);
begin

end;

procedure TfmMain.FilterChange(Sender: TObject);
begin

end;

procedure TfmMain.GroupBox1Click(Sender: TObject);
begin

end;

procedure TfmMain.imLogoClick(Sender: TObject);
begin

end;

procedure TfmMain.ExitClick(Sender: TObject);
begin

end;

// HKCU\Software\SAMP\gta_sa_exe is where the stock launcher keeps it, ask and
// remember when it is missing
function TfmMain.GetGtaExePath: string;
{$IFDEF WINDOWS}
var
  Reg: TRegistry;
  Dlg: TOpenDialog;
{$ENDIF}
begin
  Result := '';
{$IFDEF WINDOWS}
  Reg := TRegistry.Create;
  try
    Reg.RootKey := HKEY_CURRENT_USER;
    if Reg.OpenKeyReadOnly('Software\SAMP') then
    begin
      if Reg.ValueExists('gta_sa_exe') then
        Result := Reg.ReadString('gta_sa_exe');
      Reg.CloseKey;
    end;
  finally
    Reg.Free;
  end;

  if FileExists(Result) then
    Exit;

  Dlg := TOpenDialog.Create(Self);
  try
    Dlg.Title := 'Locate gta_sa.exe';
    Dlg.Filter := 'GTA: San Andreas|gta_sa.exe|All files|*.*';
    Dlg.Options := Dlg.Options + [ofFileMustExist];
    if not Dlg.Execute then
      Exit('');
    Result := Dlg.FileName;
  finally
    Dlg.Free;
  end;

  Reg := TRegistry.Create;
  try
    Reg.RootKey := HKEY_CURRENT_USER;
    if Reg.OpenKey('Software\SAMP', True) then
    begin
      Reg.WriteString('gta_sa_exe', Result);
      Reg.CloseKey;
    end;
  finally
    Reg.Free;
  end;
{$ENDIF}
end;

// same shape as the Delphi ServerConnect: start the game suspended, inject
// samp.dll, then let it run
procedure TfmMain.ServerConnect(const AHost: string; APort: Word;
  const APassword: string);
{$IFDEF WINDOWS}
var
  sExe, sDir, sCmd, sDll: string;
  StartInfo: TStartupInfo;
  ProcInfo: TProcessInformation;
  pRemote: Pointer;
  hThread: THandle;
  dwWritten: SIZE_T;
  dwTid: DWORD;
  pLoadLibrary: Pointer;
{$ENDIF}
begin
{$IFDEF WINDOWS}
  sExe := GetGtaExePath;
  if not FileExists(sExe) then
  begin
    MessageDlg('GTA: San Andreas executable not found, aborting launch.',
      mtError, [mbOK], 0);
    Exit;
  end;

  sDir := ExtractFilePath(sExe);
  sDll := sDir + 'samp.dll';
  if not FileExists(sDll) then
  begin
    MessageDlg('samp.dll is not next to gta_sa.exe:' + LineEnding + sDll,
      mtError, [mbOK], 0);
    Exit;
  end;

  sCmd := Format('"%s" -c -n %s -h %s -p %d', [sExe, lebName.Text, AHost, APort]);
  if APassword <> '' then
    sCmd := sCmd + ' -z ' + APassword;

  FillChar(StartInfo, SizeOf(StartInfo), 0);
  FillChar(ProcInfo, SizeOf(ProcInfo), 0);
  StartInfo.cb := SizeOf(StartInfo);

  if not CreateProcess(nil, PChar(sCmd), nil, nil, False,
      CREATE_NEW_PROCESS_GROUP or NORMAL_PRIORITY_CLASS or CREATE_SUSPENDED,
      nil, PChar(sDir), StartInfo, ProcInfo) then
  begin
    MessageDlg('Unable to launch the game.', mtError, [mbOK], 0);
    Exit;
  end;

  pLoadLibrary := GetProcAddress(GetModuleHandle('kernel32'), 'LoadLibraryA');
  pRemote := VirtualAllocEx(ProcInfo.hProcess, nil, Length(sDll) + 1,
    MEM_COMMIT, PAGE_READWRITE);

  if (pRemote = nil) or (pLoadLibrary = nil) then
  begin
    // nothing was injected, so do not leave a plain GTA running
    TerminateProcess(ProcInfo.hProcess, 0);
    CloseHandle(ProcInfo.hThread);
    CloseHandle(ProcInfo.hProcess);
    MessageDlg('Could not prepare samp.dll injection.', mtError, [mbOK], 0);
    Exit;
  end;

  WriteProcessMemory(ProcInfo.hProcess, pRemote, PChar(sDll),
    Length(sDll) + 1, dwWritten);
  hThread := CreateRemoteThread(ProcInfo.hProcess, nil, 0, pLoadLibrary,
    pRemote, 0, dwTid);

  if hThread = 0 then
  begin
    VirtualFreeEx(ProcInfo.hProcess, pRemote, 0, MEM_RELEASE);
    TerminateProcess(ProcInfo.hProcess, 0);
    CloseHandle(ProcInfo.hThread);
    CloseHandle(ProcInfo.hProcess);
    MessageDlg('samp.dll injection failed.', mtError, [mbOK], 0);
    Exit;
  end;

  WaitForSingleObject(hThread, 5000);
  CloseHandle(hThread);
  VirtualFreeEx(ProcInfo.hProcess, pRemote, 0, MEM_RELEASE);
  ResumeThread(ProcInfo.hThread);
  CloseHandle(ProcInfo.hThread);
  CloseHandle(ProcInfo.hProcess);
{$ELSE}
  MessageDlg('Launching the game is only supported on Windows.',
    mtError, [mbOK], 0);
{$ENDIF}
end;

procedure TfmMain.ConnectClick(Sender: TObject);
var
  iSel: Integer;
  sNick, sPwd: string;
begin
  iSel := SelectedServer;
  if iSel < 0 then
  begin
    MessageDlg('Select a server first.', mtInformation, [mbOK], 0);
    Exit;
  end;

  if Trim(lebName.Text) = '' then
  begin
    sNick := '';
    if not InputQuery('Who are you?', 'Enter your nickname/handle...', sNick) then
      Exit;
    if Trim(sNick) = '' then
      Exit;
    lebName.Text := Trim(sNick);
  end;

  sPwd := '';
  if Servers[iSel].Passworded then
    if not InputQuery('Server Password', 'This server requires a password...', sPwd) then
      Exit;

  ServerConnect(Servers[iSel].Address, Servers[iSel].Port, sPwd);
end;

procedure TfmMain.CopyServerInfoClick(Sender: TObject);
begin

end;

procedure TfmMain.DeleteServerClick(Sender: TObject);
var
  i, iSel: Integer;
begin
  iSel := lvServers.ItemIndex;
  if (iSel < 0) or (iSel > High(Servers)) then
    Exit;

  for i := iSel to High(Servers) - 1 do
    Servers[i] := Servers[i + 1];
  SetLength(Servers, Length(Servers) - 1);

  UpdateServerList;
  UpdateSelectionUI;
end;

procedure TfmMain.edFilterModeClick(Sender: TObject);
begin

end;

procedure TfmMain.AddServerClick(Sender: TObject);
var
  sInput, sHost: string;
  wPort: Word;
begin
  sInput := '';
  if not InputQuery('Add Server', 'Enter new server HOST:PORT...', sInput) then
    Exit;

  SplitHostPort(sInput, sHost, wPort);
  if sHost = '' then
    Exit;

  if FindServer(sHost, wPort) >= 0 then
  begin
    MessageDlg('This server is already on your list.', mtError, [mbOK], 0);
    Exit;
  end;

  SetLength(Servers, Length(Servers) + 1);
  with Servers[High(Servers)] do
  begin
    Address := sHost;
    Port := wPort;
    // nothing queries the server yet, so show what was typed
    HostName := Format('(No info) %s:%d', [sHost, wPort]);
    Players := 0;
    MaxPlayers := 0;
    Ping := 0;
    Mode := '';
    Language := '';
    Passworded := False;
  end;

  UpdateServerList;
  lvServers.ItemIndex := High(Servers);
  UpdateSelectionUI;
end;

procedure TfmMain.AboutClick(Sender: TObject);
begin

end;

end.

