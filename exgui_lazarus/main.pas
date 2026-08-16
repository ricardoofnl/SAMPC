unit Main;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, ComCtrls, Menus,
  ExtCtrls, StdCtrls, ValEdit, Grids, TAGraph, Types;

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
  private
    // favourites, held in memory only, nothing queries or persists them yet
    Servers: array of TServerEntry;
    function FindServer(const AAddress: string; APort: Word): Integer;
    procedure UpdateServerList;
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

procedure TfmMain.ConnectClick(Sender: TObject);
begin

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
end;

procedure TfmMain.AboutClick(Sender: TObject);
begin

end;

end.

