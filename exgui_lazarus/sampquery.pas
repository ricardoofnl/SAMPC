unit SampQuery;

{$mode objfpc}{$H+}

// The SA-MP browser query protocol, ported from TfmMain.QueryServerInfo in the
// Delphi original under exgui/. A request is
//
//   'SAMP' + ip[4] + port[2 little endian] + opcode
//
// which is 11 bytes, except the ping opcode which appends a 4 byte tick and is
// 15. The server echoes those first 11 bytes back so a reply can be matched to
// the row that asked for it.

interface

uses
  SysUtils
  // winsock2 last, the windows unit carries older conflicting socket types
  {$IFDEF WINDOWS}, Windows, WinSock2{$ENDIF};

const
  QUERY_OP_INFO    = 'i';
  QUERY_OP_RULES   = 'r';
  QUERY_OP_CLIENTS = 'c';
  QUERY_OP_PING    = 'p';

  QUERY_HEADER_LEN = 11;
  QUERY_BUFFER_LEN = 8192;

type
  TQueryBuffer = array[0..QUERY_BUFFER_LEN - 1] of Byte;

function QueryOpen(out ASocket: TSocket): Boolean;
procedure QueryClose(var ASocket: TSocket);

// dotted quad or host name, returns the address in network order, 0 on failure
function QueryResolve(const AHost: string): LongWord;

function QuerySend(ASocket: TSocket; ABinAddr: LongWord; APort: Word;
  AOpcode: AnsiChar; ATick: LongWord): Boolean;

// non blocking, True only when a datagram was actually read
function QueryReceive(ASocket: TSocket; out ABuf: TQueryBuffer;
  out ALen: Integer): Boolean;

function QueryTick: LongWord;

implementation

{$IFDEF WINDOWS}
var
  bWinsockReady: Boolean = False;
{$ENDIF}

function QueryTick: LongWord;
begin
{$IFDEF WINDOWS}
  Result := GetTickCount;
{$ELSE}
  Result := LongWord(Trunc(Now * 86400000));
{$ENDIF}
end;

function QueryOpen(out ASocket: TSocket): Boolean;
{$IFDEF WINDOWS}
var
  WSAData: TWSAData;
  Addr: TSockAddrIn;
  dwNonBlocking: LongWord;
{$ENDIF}
begin
  Result := False;
  ASocket := INVALID_SOCKET;

{$IFDEF WINDOWS}
  if not bWinsockReady then
  begin
    if WSAStartup($0202, WSAData) <> 0 then Exit;
    bWinsockReady := True;
  end;

  ASocket := Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if ASocket = INVALID_SOCKET then Exit;

  FillChar(Addr, SizeOf(Addr), 0);
  Addr.sin_family := AF_INET;
  Addr.sin_addr.S_addr := INADDR_ANY;
  Addr.sin_port := 0;

  if Bind(ASocket, Addr, SizeOf(Addr)) = SOCKET_ERROR then
  begin
    CloseSocket(ASocket);
    ASocket := INVALID_SOCKET;
    Exit;
  end;

  // polled from a timer rather than through WSAAsyncSelect, so it must never
  // block the ui thread
  dwNonBlocking := 1;
  ioctlsocket(ASocket, LongInt(FIONBIO), @dwNonBlocking);

  Result := True;
{$ENDIF}
end;

procedure QueryClose(var ASocket: TSocket);
begin
{$IFDEF WINDOWS}
  if ASocket <> INVALID_SOCKET then
  begin
    CloseSocket(ASocket);
    ASocket := INVALID_SOCKET;
  end;
{$ENDIF}
end;

function QueryResolve(const AHost: string): LongWord;
{$IFDEF WINDOWS}
var
  pEntry: PHostEnt;
{$ENDIF}
begin
  Result := 0;
{$IFDEF WINDOWS}
  if AHost = '' then Exit;

  Result := inet_addr(PAnsiChar(AnsiString(AHost)));
  if Result <> LongWord(INADDR_NONE) then Exit;

  pEntry := gethostbyname(PAnsiChar(AnsiString(AHost)));
  if (pEntry = nil) or (pEntry^.h_addr_list = nil) then
  begin
    Result := 0;
    Exit;
  end;

  Result := PLongWord(pEntry^.h_addr_list^)^;
{$ENDIF}
end;

function QuerySend(ASocket: TSocket; ABinAddr: LongWord; APort: Word;
  AOpcode: AnsiChar; ATick: LongWord): Boolean;
{$IFDEF WINDOWS}
var
  Buf: array[0..14] of Byte;
  Addr: TSockAddrIn;
  iLen: Integer;
{$ENDIF}
begin
  Result := False;
{$IFDEF WINDOWS}
  if (ASocket = INVALID_SOCKET) or (ABinAddr = 0) then Exit;

  Buf[0] := Ord('S');
  Buf[1] := Ord('A');
  Buf[2] := Ord('M');
  Buf[3] := Ord('P');

  // the address goes out in network order, which is already how it is stored
  Move(ABinAddr, Buf[4], 4);
  Move(APort, Buf[8], 2);

  Buf[10] := Ord(AOpcode);
  iLen := QUERY_HEADER_LEN;

  if AOpcode = QUERY_OP_PING then
  begin
    Move(ATick, Buf[11], 4);
    iLen := 15;
  end;

  FillChar(Addr, SizeOf(Addr), 0);
  Addr.sin_family := AF_INET;
  Addr.sin_addr.S_addr := ABinAddr;
  Addr.sin_port := htons(APort);

  Result := sendto(ASocket, Buf, iLen, 0, Addr, SizeOf(Addr)) = iLen;
{$ENDIF}
end;

function QueryReceive(ASocket: TSocket; out ABuf: TQueryBuffer;
  out ALen: Integer): Boolean;
{$IFDEF WINDOWS}
var
  Addr: TSockAddrIn;
  iAddrLen: Integer;
{$ENDIF}
begin
  Result := False;
  ALen := 0;
{$IFDEF WINDOWS}
  if ASocket = INVALID_SOCKET then Exit;

  iAddrLen := SizeOf(Addr);
  ALen := recvfrom(ASocket, ABuf, SizeOf(ABuf), 0, Addr, iAddrLen);

  if ALen <= 0 then
  begin
    ALen := 0;
    Exit;
  end;

  Result := True;
{$ENDIF}
end;

end.