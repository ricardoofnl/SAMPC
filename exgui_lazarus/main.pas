unit Main;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, ComCtrls, Menus,
  ExtCtrls, StdCtrls, ValEdit, Grids, TAGraph, TASeries, Types, SampQuery
  {$IFDEF WINDOWS}, Windows, WinSock2, Registry{$ENDIF};

type

  TQueryPlayer = record
    Name: string;
    Score: Integer;
  end;

  TQueryRule = record
    Name: string;
    Value: string;
  end;

  // subset of the Delphi TServerInfo, only what the browser shows
  TServerEntry = record
    Address: string;
    Port: Word;
    HostName: string;
    Players: Integer;
    MaxPlayers: Integer;
    Ping: Integer;
    Mode: string;
    Map: string;
    Passworded: Boolean;
    ServerPassword: string;
    RconPassword: string;
    aPlayers: array of TQueryPlayer;
    aRules: array of TQueryRule;
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
    procedure FormDestroy(Sender: TObject);
    procedure lebNameChange(Sender: TObject);
    procedure lvServersSelectItem(Sender: TObject; Item: TListItem;
      Selected: Boolean);
  private
    // favourites, loaded from and written back to USERDATA.DAT
    Servers: array of TServerEntry;
    QuerySocket: TSocket;
    QueryQueue: TStringList;
    tmrQuery: TTimer;
    function FindServer(const AAddress: string; APort: Word): Integer;
    procedure QueueServer(AIndex: Integer);
    procedure SendQueries(AIndex: Integer; APing, AInfo, AClients, ARules: Boolean);
    procedure ParseQueryReply(const ABuf: TQueryBuffer; ALen: Integer);
    procedure PollQueries(Sender: TObject);
    procedure UpdateDetailViews;
    function ReadPlayerName: string;
    procedure WritePlayerName(const AName: string);
    function UserDataPath: string;
    procedure LoadFavorites(const AFileName: string; AAppend: Boolean);
    procedure SaveFavorites(const AFileName: string);
    procedure UpdateServerList;
    procedure UpdateSelectionUI;
    function SelectedServer: Integer;
    function ReadGtaExePath: string;
    procedure WriteGtaExePath(const APath: string);
    function BrowseForGtaExe(const AStart: string): string;
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
  if Servers[iSel].Map <> '' then
    edSIMap.Text := Servers[iSel].Map
  else
    edSIMap.Text := '- - -';
  gbInfo.Caption := ' ' + Servers[iSel].HostName + ' ';

  UpdateDetailViews;
end;

procedure TfmMain.lvServersSelectItem(Sender: TObject; Item: TListItem;
  Selected: Boolean);
begin
  UpdateSelectionUI;
end;

procedure TfmMain.FormCreate(Sender: TObject);
begin
  QuerySocket := INVALID_SOCKET;
  QueryQueue := TStringList.Create;

  QueryOpen(QuerySocket);

  // the delphi version drives this from WSAAsyncSelect, a timer keeps the
  // socket handling out of the window procedure
  tmrQuery := TTimer.Create(Self);
  tmrQuery.Interval := 100;
  tmrQuery.OnTimer := @PollQueries;
  tmrQuery.Enabled := True;

  lebName.Text := ReadPlayerName;

  LoadFavorites(UserDataPath, False);
  UpdateServerList;
  UpdateSelectionUI;
end;

// written as it is typed, a killed process should not cost the nickname
procedure TfmMain.lebNameChange(Sender: TObject);
begin
  WritePlayerName(lebName.Text);
end;

procedure TfmMain.FormDestroy(Sender: TObject);
begin
  WritePlayerName(lebName.Text);
  SaveFavorites(UserDataPath);

  if tmrQuery <> nil then
    tmrQuery.Enabled := False;

  QueryClose(QuerySocket);
  FreeAndNil(QueryQueue);
end;

//----------------------------------------------------------------------------
// query

procedure TfmMain.QueueServer(AIndex: Integer);
begin
  if (AIndex < 0) or (AIndex > High(Servers)) then Exit;
  if QueryQueue = nil then Exit;

  if QueryQueue.IndexOf(IntToStr(AIndex)) < 0 then
    QueryQueue.Add(IntToStr(AIndex));
end;

procedure TfmMain.SendQueries(AIndex: Integer; APing, AInfo, AClients, ARules: Boolean);
var
  dwAddr: LongWord;
begin
  if (AIndex < 0) or (AIndex > High(Servers)) then Exit;
  if QuerySocket = INVALID_SOCKET then Exit;

  dwAddr := QueryResolve(Servers[AIndex].Address);
  if dwAddr = 0 then Exit;

  if AInfo then
    QuerySend(QuerySocket, dwAddr, Servers[AIndex].Port, QUERY_OP_INFO, 0);
  if AClients then
    QuerySend(QuerySocket, dwAddr, Servers[AIndex].Port, QUERY_OP_CLIENTS, 0);
  if ARules then
    QuerySend(QuerySocket, dwAddr, Servers[AIndex].Port, QUERY_OP_RULES, 0);
  if APing then
    QuerySend(QuerySocket, dwAddr, Servers[AIndex].Port, QUERY_OP_PING, QueryTick);
end;

// mirrors TfmMain.QueryServerInfoParse, every string is length prefixed and is
// not terminated
procedure TfmMain.ParseQueryReply(const ABuf: TQueryBuffer; ALen: Integer);
var
  sIP: string;
  wPort: Word;
  iIdx, iPos, i, iCount: Integer;
  dwTick: LongWord;

  function ReadStr(ALenBytes: Integer): string;
  var
    n: LongWord;
  begin
    Result := '';
    n := 0;
    if ALenBytes = 4 then
    begin
      if iPos + 4 > ALen then Exit;
      Move(ABuf[iPos], n, 4);
      Inc(iPos, 4);
    end
    else
    begin
      if iPos + 1 > ALen then Exit;
      n := ABuf[iPos];
      Inc(iPos, 1);
    end;
    if (n = 0) or (iPos + Integer(n) > ALen) then Exit;
    SetLength(Result, n);
    Move(ABuf[iPos], Result[1], n);
    Inc(iPos, Integer(n));
  end;

begin
  // 4 magic, 4 ip, 2 port, 1 opcode
  if ALen < QUERY_HEADER_LEN then Exit;
  if (ABuf[0] <> Ord('S')) or (ABuf[1] <> Ord('A')) or
     (ABuf[2] <> Ord('M')) or (ABuf[3] <> Ord('P')) then Exit;

  sIP := Format('%d.%d.%d.%d', [ABuf[4], ABuf[5], ABuf[6], ABuf[7]]);
  Move(ABuf[8], wPort, 2);

  iIdx := FindServer(sIP, wPort);
  if iIdx < 0 then Exit;

  iPos := QUERY_HEADER_LEN;

  case Chr(ABuf[10]) of
    QUERY_OP_INFO:
      begin
        if iPos + 5 > ALen then Exit;
        Servers[iIdx].Passworded := ABuf[iPos] <> 0;
        Inc(iPos, 1);
        Move(ABuf[iPos], wPort, 2);
        Servers[iIdx].Players := wPort;
        Inc(iPos, 2);
        Move(ABuf[iPos], wPort, 2);
        Servers[iIdx].MaxPlayers := wPort;
        Inc(iPos, 2);

        Servers[iIdx].HostName := ReadStr(4);
        Servers[iIdx].Mode := ReadStr(4);
        Servers[iIdx].Map := ReadStr(4);

        // the favourites file only had the address until now
        SaveFavorites(UserDataPath);
      end;

    QUERY_OP_CLIENTS:
      begin
        if iPos + 2 > ALen then Exit;
        Move(ABuf[iPos], wPort, 2);
        Inc(iPos, 2);
        iCount := wPort;

        SetLength(Servers[iIdx].aPlayers, iCount);
        for i := 0 to iCount - 1 do
        begin
          Servers[iIdx].aPlayers[i].Name := ReadStr(1);
          if iPos + 4 > ALen then
          begin
            SetLength(Servers[iIdx].aPlayers, i);
            Break;
          end;
          Move(ABuf[iPos], Servers[iIdx].aPlayers[i].Score, 4);
          Inc(iPos, 4);
        end;
      end;

    QUERY_OP_RULES:
      begin
        if iPos + 2 > ALen then Exit;
        Move(ABuf[iPos], wPort, 2);
        Inc(iPos, 2);
        iCount := wPort;

        SetLength(Servers[iIdx].aRules, iCount);
        for i := 0 to iCount - 1 do
        begin
          Servers[iIdx].aRules[i].Name := ReadStr(1);
          Servers[iIdx].aRules[i].Value := ReadStr(1);
        end;
      end;

    QUERY_OP_PING:
      begin
        if ALen < 15 then Exit;
        Move(ABuf[11], dwTick, 4);
        Servers[iIdx].Ping := Integer(QueryTick - dwTick);
        if Servers[iIdx].Ping < 1 then
          Servers[iIdx].Ping := 1;
      end;
  end;

  UpdateServerList;
  if SelectedServer = iIdx then
    UpdateSelectionUI;
end;

procedure TfmMain.PollQueries(Sender: TObject);
var
  Buf: TQueryBuffer;
  iLen, iIdx, iGuard: Integer;
begin
  // one queued server per tick keeps a long favourites list from flooding
  if (QueryQueue <> nil) and (QueryQueue.Count > 0) then
  begin
    iIdx := StrToIntDef(QueryQueue[0], -1);
    QueryQueue.Delete(0);
    SendQueries(iIdx, True, True, True, True);
  end;

  // drain whatever came back since the last tick
  iGuard := 0;
  while QueryReceive(QuerySocket, Buf, iLen) and (iGuard < 64) do
  begin
    ParseQueryReply(Buf, iLen);
    Inc(iGuard);
  end;
end;

procedure TfmMain.UpdateDetailViews;
var
  iSel, i: Integer;
  Item: TListItem;
begin
  iSel := SelectedServer;

  lvPlayers.BeginUpdate;
  lvRules.BeginUpdate;
  try
    lvPlayers.Items.Clear;
    lvRules.Items.Clear;
    if iSel < 0 then Exit;

    for i := 0 to High(Servers[iSel].aPlayers) do
    begin
      Item := lvPlayers.Items.Add;
      Item.Caption := Servers[iSel].aPlayers[i].Name;
      Item.SubItems.Add(IntToStr(Servers[iSel].aPlayers[i].Score));
    end;

    for i := 0 to High(Servers[iSel].aRules) do
    begin
      Item := lvRules.Items.Add;
      Item.Caption := Servers[iSel].aRules[i].Name;
      Item.SubItems.Add(Servers[iSel].aRules[i].Value);
    end;
  finally
    lvRules.EndUpdate;
    lvPlayers.EndUpdate;
  end;
end;

//----------------------------------------------------------------------------
// persistence

function TfmMain.ReadPlayerName: string;
{$IFDEF WINDOWS}
var
  Reg: TRegistry;
{$ENDIF}
begin
  Result := '';
{$IFDEF WINDOWS}
  Reg := TRegistry.Create;
  try
    Reg.RootKey := HKEY_CURRENT_USER;
    if Reg.OpenKeyReadOnly('Software\SAMP') then
    begin
      if Reg.ValueExists('PlayerName') then
        Result := Reg.ReadString('PlayerName');
      Reg.CloseKey;
    end;
  finally
    Reg.Free;
  end;
{$ENDIF}
end;

procedure TfmMain.WritePlayerName(const AName: string);
{$IFDEF WINDOWS}
var
  Reg: TRegistry;
{$ENDIF}
begin
{$IFDEF WINDOWS}
  Reg := TRegistry.Create;
  try
    Reg.RootKey := HKEY_CURRENT_USER;
    if Reg.OpenKey('Software\SAMP', True) then
    begin
      Reg.WriteString('PlayerName', AName);
      Reg.CloseKey;
    end;
  finally
    Reg.Free;
  end;
{$ENDIF}
end;

function TfmMain.UserDataPath: string;
begin
  Result := ExtractFilePath(ParamStr(0)) + 'USERDATA.DAT';
end;

// the binary layout ImportFavorites and ExportFavorites use in the Delphi
// original: tag SAMP, int32 version, int32 count, then per server the address,
// port, hostname, server password and rcon password
procedure TfmMain.LoadFavorites(const AFileName: string; AAppend: Boolean);
var
  Stream: TFileStream;
  FileTag: array[0..3] of AnsiChar;
  iVersion, iCount, iLen, i, iBase: Integer;

  function ReadStr: string;
  var
    n: Integer;
  begin
    Result := '';
    n := 0;
    if Stream.Read(n, 4) <> 4 then Exit;
    if (n <= 0) or (n > 1024) then Exit;
    SetLength(Result, n);
    Stream.ReadBuffer(Result[1], n);
  end;

begin
  if not FileExists(AFileName) then Exit;

  Stream := TFileStream.Create(AFileName, fmOpenRead or fmShareDenyNone);
  try
    if Stream.Read(FileTag[0], 4) <> 4 then Exit;
    if (FileTag[0] <> 'S') or (FileTag[1] <> 'A') or (FileTag[2] <> 'M') or (FileTag[3] <> 'P') then
    begin
      MessageDlg('Invalid SA:MP file.', mtError, [mbOK], 0);
      Exit;
    end;

    iVersion := 0;
    if Stream.Read(iVersion, 4) <> 4 then Exit;
    if iVersion <> 1 then
    begin
      MessageDlg('Bad SA:MP favorites file version.', mtError, [mbOK], 0);
      Exit;
    end;

    iCount := 0;
    if Stream.Read(iCount, 4) <> 4 then Exit;

    if not AAppend then
      SetLength(Servers, 0);

    for i := 1 to iCount do
    begin
      if Stream.Position >= Stream.Size then Break;

      iBase := Length(Servers);
      SetLength(Servers, iBase + 1);

      Servers[iBase].Address := ReadStr;
      iLen := 0;
      Stream.Read(iLen, 4);
      Servers[iBase].Port := Word(iLen);
      Servers[iBase].HostName := ReadStr;
      Servers[iBase].ServerPassword := ReadStr;
      Servers[iBase].RconPassword := ReadStr;

      Servers[iBase].Ping := 0;
      Servers[iBase].Players := 0;
      Servers[iBase].MaxPlayers := 0;

      // a duplicate address collapses back onto the existing row
      if (Servers[iBase].Address = '') or
         (FindServer(Servers[iBase].Address, Servers[iBase].Port) <> iBase) then
        SetLength(Servers, iBase)
      else
        QueueServer(iBase);
    end;
  finally
    Stream.Free;
  end;
end;

procedure TfmMain.SaveFavorites(const AFileName: string);
var
  Stream: TFileStream;
  FileTag: array[0..3] of AnsiChar = ('S', 'A', 'M', 'P');
  iValue, i: Integer;

  procedure WriteStr(const AText: string);
  var
    n: Integer;
  begin
    n := Length(AText);
    Stream.WriteBuffer(n, 4);
    if n > 0 then
      Stream.WriteBuffer(AText[1], n);
  end;

begin
  Stream := TFileStream.Create(AFileName, fmCreate);
  try
    Stream.WriteBuffer(FileTag[0], 4);
    iValue := 1;
    Stream.WriteBuffer(iValue, 4);
    iValue := Length(Servers);
    Stream.WriteBuffer(iValue, 4);

    for i := 0 to High(Servers) do
    begin
      WriteStr(Servers[i].Address);
      iValue := Servers[i].Port;
      Stream.WriteBuffer(iValue, 4);
      WriteStr(Servers[i].HostName);
      WriteStr(Servers[i].ServerPassword);
      WriteStr(Servers[i].RconPassword);
    end;
  finally
    Stream.Free;
  end;
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
      Item.SubItems.Add(Servers[i].Map);
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
var
  sInput, sHost: string;
  wPort: Word;
begin
  sInput := '';
  if not InputQuery('Quick Connect', 'Enter server HOST:PORT...', sInput) then
    Exit;

  SplitHostPort(sInput, sHost, wPort);
  if sHost = '' then
    Exit;

  ServerConnect(sHost, wPort, '');
end;

procedure TfmMain.RefreshServerClick(Sender: TObject);
var
  iSel: Integer;
begin
  iSel := SelectedServer;
  if iSel < 0 then
  begin
    // nothing picked, refresh the whole list the way the toolbar implies
    for iSel := 0 to High(Servers) do
      QueueServer(iSel);
    Exit;
  end;

  SendQueries(iSel, True, True, True, True);
end;

procedure TfmMain.ServerPropertiesClick(Sender: TObject);
begin

end;

// the only setting so far is which gta_sa.exe Connect launches, and without a way
// to change it a stale registry entry keeps sending you back to the old install
procedure TfmMain.SettingsClick(Sender: TObject);
var
  sCurrent, sNew, sDll: string;
begin
  sCurrent := ReadGtaExePath;

  if sCurrent = '' then
    sCurrent := '(not set)';
  if MessageDlg('Game Directory',
      'Connect currently launches:' + LineEnding + LineEnding +
      sCurrent + LineEnding + LineEnding +
      'Pick a different gta_sa.exe?',
      mtConfirmation, [mbYes, mbNo], 0) <> mrYes then
    Exit;

  sNew := BrowseForGtaExe(ReadGtaExePath);
  if sNew = '' then
    Exit;

  // samp.dll has to sit beside it, Connect injects it from there
  sDll := ExtractFilePath(sNew) + 'samp.dll';
  if not FileExists(sDll) then
    if MessageDlg('samp.dll is not in that directory:' + LineEnding + sDll +
        LineEnding + LineEnding + 'Save the path anyway?',
        mtWarning, [mbYes, mbNo], 0) <> mrYes then
      Exit;

  WriteGtaExePath(sNew);
  MessageDlg('Game directory set to:' + LineEnding + ExtractFileDir(sNew),
    mtInformation, [mbOK], 0);
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

// HKCU\Software\SAMP\gta_sa_exe is where the stock launcher keeps it
function TfmMain.ReadGtaExePath: string;
{$IFDEF WINDOWS}
var
  Reg: TRegistry;
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
{$ENDIF}
end;

procedure TfmMain.WriteGtaExePath(const APath: string);
{$IFDEF WINDOWS}
var
  Reg: TRegistry;
{$ENDIF}
begin
{$IFDEF WINDOWS}
  Reg := TRegistry.Create;
  try
    Reg.RootKey := HKEY_CURRENT_USER;
    if Reg.OpenKey('Software\SAMP', True) then
    begin
      Reg.WriteString('gta_sa_exe', APath);
      Reg.CloseKey;
    end;
  finally
    Reg.Free;
  end;
{$ENDIF}
end;

function TfmMain.BrowseForGtaExe(const AStart: string): string;
var
  Dlg: TOpenDialog;
begin
  Result := '';
  Dlg := TOpenDialog.Create(Self);
  try
    Dlg.Title := 'Locate gta_sa.exe';
    Dlg.Filter := 'GTA: San Andreas|gta_sa.exe|All files|*.*';
    Dlg.Options := Dlg.Options + [ofFileMustExist];
    if AStart <> '' then
      Dlg.InitialDir := ExtractFileDir(AStart);
    if Dlg.Execute then
      Result := Dlg.FileName;
  finally
    Dlg.Free;
  end;
end;

// only prompts when the stored path is unusable, use Settings to change it
function TfmMain.GetGtaExePath: string;
begin
  Result := ReadGtaExePath;
  if FileExists(Result) then
    Exit;

  Result := BrowseForGtaExe(Result);
  if Result <> '' then
    WriteGtaExePath(Result);
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

  // the queue holds indexes, which have all just shifted
  if QueryQueue <> nil then
    QueryQueue.Clear;

  SaveFavorites(UserDataPath);

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
    // replaced by the hostname as soon as the info reply lands
    HostName := Format('(Retrieving info...) %s:%d', [sHost, wPort]);
    Players := 0;
    MaxPlayers := 0;
    Ping := 0;
    Mode := '';
    Map := '';
    Passworded := False;
    ServerPassword := '';
    RconPassword := '';
  end;

  QueueServer(High(Servers));
  SaveFavorites(UserDataPath);

  UpdateServerList;
  lvServers.ItemIndex := High(Servers);
  UpdateSelectionUI;
end;

procedure TfmMain.AboutClick(Sender: TObject);
begin

end;

end.

