{WERT Sim-Card Scanner. 
 Copyright (C) 2004-2005 by Wert (AKA Joda).

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 Project home root - ftp://ftp.ionb.ru/pub/sim/
 Author's e-mail   - wert@ionb.ru}

unit ComPort;
 {Модуль WertScan-а с функциями управления коммуникационным портом}
 
interface
 uses
  WTypes,Synaser;
 function PortInit(var Port:TBlockSerial;PortName:string;PortSpeed:Longint;Parity:Char;StopBits:byte):Longint;
 function IsDevicePresent(var Port:TBlockSerial):Boolean;
 function ResetCard(var Port:TBlockSerial):Longint;
 function ReadCard(var Port:TBlockSerial;var Buffer:TBuffer;Count:Longint):Longint; 
 function WriteCard(var Port:TBlockSerial;Buffer:TBuffer;Count:Longint):Longint; 
 function RunCommand(var Port:TBlockSerial;Command,P1,P2,P3:byte;Parameter:TBuffer;var Error:Byte):TBuffer;
 function GetCHVStatus(var Port:TBlockSerial):Longint;
 function VerifyCHV(var Port:TBlockSerial;Num:byte;Pin:String8):Longint;
 function Gsm(var Port:TBlockSerial;Rand:TData16;var SRes:TData12):Longint;
 function ReadIMSI(var Port:TBlockSerial;var IMSI:String18):Longint;
 function migalka(var Port:TBlockSerial;sl1:longint;sl2:longint):Longint;
 function ReadICCID(var Port:TBlockSerial;var ICCID:String19):Longint;
 function ReadSIMInfo(var Port:TBlockSerial;ReadExist:Boolean):Longint;
 function ViewSMS(var Port:TBlockSerial):Longint;
 function ViewADN(var Port:TBlockSerial):Longint;
 function ReadInitLog(var Port:TBlockSerial;Seconds:Longword):Longint;
 function BootAuth(var Port:TBlockSerial;Timeout:Longint;Applet:string):Longint;
 function AppletTest(var Port:TBlockSerial):Longint;
  
implementation
 uses
 unix, glib2,
 {$IFDEF LINUX}
 {$ELSE}
  Windows,
 {$ENDIF}
  SynaUtil,SysUtils,Crt,
  Tables,Functions,Emulator,SmsAdn;
  
 const
  RESETTIME:Longint=500;               {Время поднятия сигнала RTS при сбросе, ms}
  POSTRESETTIME:Longint=200;           {Время ожидания после сброса карты, ms}
  TIMEOUT:Longint=3000;                {Время ожидания ответа СИМ-ки - 3 сек, ms} 
  READ_TIMEOUT:Longint=5;              {Время ожидания одного символа, ms}
  READ_CONST_TIMEOUT:Longint=100;      {Время ожидания начала передачи, ms}
  
  T_DIRECT_ARR:array[0..$F] of byte=($F,$7,$B,$3,$D,$5,$9,$1,$E,$6,$A,$2,$C,$4,$8,$0);
  MarthaID_R520m:string=Chr($5C)+Chr($06)+Chr($01)+Chr($FF)+Chr($FF)+Chr($FF)+Chr($FF)+Chr($FF);
  
 {Функция инициализации порта}
 {На выходе не 0, если ошибка}
 function PortInit(var Port:TBlockSerial;PortName:string;PortSpeed:Longint;Parity:Char;StopBits:byte):Longint;
  var
   Res:Longint;
   SBConst:byte;
  begin
   Res:=0;
   SBConst:=SB1andHalf;
   if StopBits=1 then SBConst:=SB1;
   if StopBits=2 then SBConst:=SB2;
   Port:=TBlockSerial.Create;
   Port.RaiseExcept:=False;
   Port.DeadlockTimeout:=10000;
   Port.LinuxLock:=True;
   Port.Connect(PortName);
   if (Port.LastError<>0) then begin
    Res:=Port.LastError;
   end
   else begin
    Port.Config(PortSpeed,8,Parity,SBConst,False,False);
    if (Port.LastError<>0) then begin
     Res:=Port.LastError;
    end;
   end; 
   PortInit:=Res;
  end;
  
 {Функция проверки наличия устройства на порту}
 function IsDevicePresent(var Port:TBlockSerial):Boolean;
  begin
   IsDevicePresent:=Port.CTS;
  end;
 
 {Функция сброса карты}
 function ResetCard(var Port:TBlockSerial):Longint; 
  var
   Res:Longint;
  begin
   Res:=0;
   Port.RTS:=True;
   Port.DTR:=False;
   Delay(RESETTIME);
   Port.DTR:=True;
   Delay(POSTRESETTIME);
   Port.RTS:=False;
   Delay(POSTRESETTIME);
   if (not Port.CanReadEx(TIMEOUT)) then begin
    Res:=1;
   end;
   ResetCard:=Res;
  end;
  
 {Функция чтения данных из карты}
 function ReadCard(var Port:TBlockSerial;var Buffer:TBuffer;Count:Longint):Longint; 
  var
   Uk:Longint;
   Pr:Boolean;
   I:Longint;
  begin
   Uk:=0;
   if Count=0 then begin
    while (Uk<=(BufferSize-1)) and (Port.CanReadEx(READ_CONST_TIMEOUT)) do begin
     Buffer[Uk]:=Port.RecvByte(READ_TIMEOUT);
     if (Port.LastError=0) then Uk:=Uk+1;
    end;
   end
   else begin
    Pr:=True;
    while (Uk<Count) and (Pr) do begin
     Buffer[Uk]:=Port.RecvByte(TIMEOUT);
     if (Port.LastError=0) then begin
      Uk:=Uk+1;
     end
     else begin
      Pr:=False;
     end; 
    end;
   end;
   if (Uk>0) and (DIRECTION=1) then begin
    {Инверсная передача}
    for I:=0 to (Uk-1) do begin
     Buffer[I]:=((T_DIRECT_ARR[Buffer[I] and $0F]) shl 4)+T_DIRECT_ARR[(Buffer[I] and $F0) shr 4];
    end;
   end;    
   ReadCard:=Uk; 
  end;
 
 {Функция записи данных в карту}
 function WriteCard(var Port:TBlockSerial;Buffer:TBuffer;Count:Longint):Longint;
  var
   Uk:Longint;
   Pr:Boolean;
   I:Longint;
  begin
   Uk:=0;
   if DIRECTION=1 then begin
    {Инверсная передача}
    for I:=0 to (Count-1) do begin
     Buffer[I]:=((T_DIRECT_ARR[Buffer[I] and $0F]) shl 4)+T_DIRECT_ARR[(Buffer[I] and $F0) shr 4];
    end;
   end;    
   Pr:=True;
   while (Uk<Count) and (Pr) do begin
    Port.SendByte(Buffer[Uk]);
    if (Port.LastError=0) then begin
     Uk:=Uk+1;
    end
    else begin
     Pr:=False;
    end;
   end;
   WriteCard:=Uk;
  end; 
 
 {Функция выполнения команды СИМ-ки}
 {Коды ошибок:}
 { 01 - ошибка записи первых 5-ти байт команды}
 { 02 - ошибка чтения локального эха первых 5-ти байт}
 { 03 - ошибка чтения первого байта ответа карты}
 { 04 - ошибка чтения второго байта ответа карты NINO}
 { 05!- длина выходных данных команды превысила размер буфера BufferSize}
 { 06 - ошибка чтения выходных данных команды NIFO}
 { 07 - ошибка чтения второго байта ответа карты NIFO}
 { 08 - ошибка записи параметров команды FINO}
 { 09 - ошибка чтения локального эха параметров команды FINO}
 { 0A - ошибка чтения второго байта ответа карты FINO}
 { 80 - FIX-ME!!! Недоделано! NIFO}
 { 81 - FIX-ME!!! Недоделано! FINO}
 function RunCommand(var Port:TBlockSerial;Command,P1,P2,P3:byte;Parameter:TBuffer;var Error:Byte):TBuffer;
  var 
   RBuffer,WBuffer:TBuffer;
   ResBuffer:TBuffer;
   ReadC,WriteC:Longint;
   U,Pr_Cmd,Pr:boolean;
   AnswerB:byte;
   ReadCount,WriteCount:Longint;
  begin
   Error:=0;
   FillChar(ResBuffer[0],BufferSize,$0);
   {Пошлем карте первые 5 байт команды}
   FillChar(WBuffer[0],BufferSize,$0);
   WBuffer[0]:=$A0;WBuffer[1]:=Command;WBuffer[2]:=P1;WBuffer[3]:=P2;WBuffer[4]:=P3;
   WriteC:=WriteCard(Port,WBuffer,5);
   if WriteC<>5 then begin
    Error:=$01;
    RunCommand:=ResBuffer;
    Exit;
   end;
   {Считаем локальное эхо}
   FillChar(RBuffer[0],BufferSize,$0);
   ReadC:=ReadCard(Port,RBuffer,5);
   if ReadC<>5 then begin
    Error:=$02;
    RunCommand:=ResBuffer;
    Exit;
   end;
   {Цикл обработки ответа карты}
   U:=True;
   while U do begin
    FillChar(RBuffer[0],BufferSize,$0);
    ReadC:=ReadCard(Port,RBuffer,1);
    if ReadC<>1 then begin
     Error:=$03;
     RunCommand:=ResBuffer;
     Exit;
    end;
    AnswerB:=RBuffer[0];
    Pr_Cmd:=False;
    if (Command=$04) or (Command=$44) or (Command=$FA) then begin
     {NoInput/NoOutput}
     Pr_Cmd:=True;
     ResBuffer[0]:=AnswerB;
     FillChar(RBuffer[0],BufferSize,$0);
     ReadC:=ReadCard(Port,RBuffer,1);
     if ReadC<>1 then begin
      Error:=$04;
      RunCommand:=ResBuffer;
      Exit;
     end;
     ResBuffer[1]:=RBuffer[0];
     U:=False;
    end;
    if (Command=$F2) or (Command=$B0) or (Command=$B2) or (Command=$C0) then begin
     {NoInput/FixedOutput}
     Pr_Cmd:=True;
     if AnswerB<>$60 then begin
      Pr:=False;
      if (AnswerB=Command) or (AnswerB=(Command+1)) then begin
       Pr:=True;
       {Считаем результаты}
       ReadCount:=P3;
       if ReadCount=0 then ReadCount:=256;
       ReadCount:=ReadCount+2;
       if ReadCount>BufferSize then begin
        Error:=$05;
        RunCommand:=ResBuffer;
        Exit;
       end;
       FillChar(RBuffer[0],BufferSize,$0);
       ReadC:=ReadCard(Port,RBuffer,ReadCount);
       if ReadC<>ReadCount then begin
        Error:=$06;
        RunCommand:=ResBuffer;
        Exit;
       end;
       ResBuffer:=RBuffer;
       U:=False;
      end;
      if (AnswerB=(not Command)) or (AnswerB=(not (Command+1))) then begin
       {FIX-ME!!! Инверсия ответа карты}
       Error:=$80;
       RunCommand:=ResBuffer;
       Exit;
      end;      
      if Pr=False then begin
       ResBuffer[0]:=AnswerB;
       FillChar(RBuffer[0],BufferSize,$0);
       ReadC:=ReadCard(Port,RBuffer,1);
       if ReadC<>1 then begin
        Error:=$07;
        RunCommand:=ResBuffer;
        Exit;
       end;
       ResBuffer[1]:=RBuffer[0];
       U:=False;
      end;
     end; 
    end;
    if (Command=$D6) or (Command=$DC) or (Command=$20) or (Command=$24) or
     (Command=$26) or (Command=$28) or (Command=$2C) or (Command=$A4) or
     (Command=$A2) or (Command=$32) or (Command=$88) then begin
     {FixedInput/NoOutput}
     Pr_Cmd:=True;
     if AnswerB<>$60 then begin
      Pr:=False;
      if (AnswerB=Command) or (AnswerB=(Command+1)) then begin
       Pr:=True;
       {Запишем параметры}
       WriteCount:=P3;
       FillChar(WBuffer[0],BufferSize,$0);
       Move(Parameter[0],WBuffer[0],WriteCount);
       WriteC:=WriteCard(Port,WBuffer,WriteCount);
       if WriteC<>WriteCount then begin
        Error:=$08;
        RunCommand:=ResBuffer;
        Exit;
       end;
       {Считаем локальное эхо}
       FillChar(RBuffer[0],BufferSize,$0);
       ReadC:=ReadCard(Port,RBuffer,WriteCount);
       if ReadC<>WriteCount then begin
        Error:=$09;
        RunCommand:=ResBuffer;
        Exit;
       end;
      end;
      if (AnswerB=(not Command)) or (AnswerB=(not (Command+1))) then begin
       {FIX-ME!!! Инверсия ответа карты}
       Error:=$81;
       RunCommand:=ResBuffer;
       Exit;
      end;
      if Pr=False then begin
       ResBuffer[0]:=AnswerB;
       FillChar(RBuffer[0],BufferSize,$0);
       ReadC:=ReadCard(Port,RBuffer,1);
       if ReadC<>1 then begin
        Error:=$0A;
        RunCommand:=ResBuffer;
        Exit;
       end;
       ResBuffer[1]:=RBuffer[0];
       U:=False;
      end;
     end;
    end; 
    if Pr_Cmd=False then begin
     {Unknown}
     ResBuffer[0]:=AnswerB;
     FillChar(RBuffer[0],BufferSize,$0);
     ReadC:=ReadCard(Port,RBuffer,0);
     if ReadC=BufferSize then ReadC:=ReadC-1;
     if ReadC<>0 then begin
      Move(RBuffer[0],ResBuffer[1],ReadC);
     end;
     U:=False;
    end;       
   end;
   RunCommand:=ResBuffer;
  end;
  
 {Функция чтения CHV статуса карты}
 function GetCHVStatus(var Port:TBlockSerial):Longint; 
  var
   RBuffer,WBuffer:TBuffer;
   Res:Longint;
   ReadCount:Longint;
   B,I:byte;
   RError:byte;
   AllCHV:byte;
  begin
   Res:=0;
   {Select DF_GSM (7F.20)}
   FillChar(WBuffer[0],BufferSize,$0);
   WBuffer[0]:=$7F;
   WBuffer[1]:=$20;

   Sleep(100);
   RBuffer:=RunCommand(Port,$A4,$00,$00,$02,WBuffer,RError);
   if RError<>0 then begin
    LWriteln('ERROR! Cannot select DF_GSM. RunCommand error '+PrintHexByte(RError)+'.',2);
    LWriteln('',2);
    GetCHVStatus:=-1;
    Exit;
   end;
   if (RBuffer[0]<>$9F) then begin
    LWriteln('ERROR! Cannot select DF_GSM. Card answer <> 9F.',2);
    LWriteln('',2);
    GetCHVStatus:=-1;
    Exit;
   end;
   FillChar(WBuffer[0],BufferSize,$0);
   ReadCount:=RBuffer[1];

   Sleep(100);
   RBuffer:=RunCommand(Port,$C0,$00,$00,ReadCount,WBuffer,RError);
   if RError<>0 then begin
    LWriteln('ERROR! Cannot select DF_GSM. RunCommand error '+PrintHexByte(RError)+'.',2);
    LWriteln('',2);
    GetCHVStatus:=-1;
    Exit;
   end;
   if ReadCount=0 then ReadCount:=256;
   if (RBuffer[ReadCount]<>$90) or (RBuffer[ReadCount+1]<>$00) then begin
    LWriteln('ERROR! Cannot select DF_GSM. Invalid card answer after C0 command.',2);
    LWriteln('',2);
    GetCHVStatus:=-1;
    Exit;
   end;
   {Разберем ответ}
   B:=(RBuffer[13] and $80) shr 7;
   if B<>0 then begin
    LWriteln('CHV1 is disabled.',2);
    Res:=$100;
   end
   else begin
    LWriteln('CHV1 is enabled.',2);
   end;
   for I:=18 to 21 do begin
    B:=(RBuffer[I] and $80) shr 7;
    if B=0 then begin
     LWriteln(Chv_Name[I-18]+' is disabled.',2);
    end
    else begin
     LWrite(Chv_Name[I-18]+' is enabled and ',2);
     B:=RBuffer[I] and $0F;
     if B<>0 then begin
      LWriteln('remaining '+IntToStr(B)+' numbers of verification.',2);
      if I=18 then Res:=Res+1;
      if I=20 then Res:=Res+$10;
     end
     else begin
      LWriteln('blocked!!!',2);
      if I=18 then Res:=Res+2;
      if I=20 then Res:=Res+$20;
     end;
    end;
   end;
   AllCHV:=RBuffer[16]-4;
   if AllCHV>0 then begin
    for I:=23 to 22+AllCHV do begin
     B:=(RBuffer[I] and $80) shr 7;
     if B=0 then begin
      LWriteln('ADM('+IntToStr(I-19)+') is disabled.',2);
     end
     else begin
      LWrite('ADM('+IntToStr(I-19)+') is enabled and ',2);
      B:=RBuffer[I] and $0F;
      if B<>0 then begin
       LWriteln('remaining '+IntToStr(B)+' numbers of verification.',2);
      end
      else begin
       LWriteln('blocked!!!',2);
      end;
     end;
    end;
   end;
   LWriteln('',2);
   GetCHVStatus:=Res;
  end;

 {Функция верификации CHV статуса карты}
 function VerifyCHV(var Port:TBlockSerial;Num:byte;Pin:String8):Longint;
  var
   Res:Longint;
   RBuffer,WBuffer:TBuffer;
   I:byte;
   RError:byte;
  begin
   Res:=0;
   FillChar(WBuffer[0],8,$FF);
   FillChar(WBuffer[8],BufferSize-8,$0);
   for I:=1 to Length(Pin) do WBuffer[I-1]:=Ord(Pin[I]);
   RBuffer:=RunCommand(Port,$20,$00,Num,$08,WBuffer,RError);
   if RError<>0 then begin
    LWriteln('ERROR! Cannot run SIM-command 20. RunCommand error '+PrintHexByte(RError)+'.',2);
    LWriteln('',2);
    VerifyCHV:=-1;
    Exit;
   end;
   if (RBuffer[0]<>$90) or (RBuffer[1]<>$00) then begin
    LWriteln('PIN'+IntToStr(Num)+' verification FILED!!!',2);
    LWriteln('',2);
    Res:=1;
   end
   else begin
    LWriteln('PIN'+IntToStr(Num)+' verification successfully!!!',2);
    LWriteln('',2);
   end;
   VerifyCHV:=Res;
  end;    
  
 {Функция выполнения GSM-algo}
 function Gsm(var Port:TBlockSerial;Rand:TData16;var SRes:TData12):Longint; 
  var
   Res:Longint;
   RBuffer,WBuffer:TBuffer;
   ReadCount:Longint;
   RError:byte;

  begin
   Res:=0;
   FillChar(SRes[0],12,0);
   if Device=1 then begin
    FillChar(WBuffer[0],BufferSize,0);
    Move(Rand[0],WBuffer[0],16);
    RBuffer:=RunCommand(Port,$88,$00,$00,$10,WBuffer,RError);
    if RError<>0 then begin
     LWriteln('ERROR! Cannot run SIM-command 88. RunCommand error '+PrintHexByte(RError)+'.',2);
     LWriteln('',2);
     Gsm:=-1;
     Exit;
    end;
    if (RBuffer[0]<>$9F) then begin
     LWriteln('ERROR! Cannot run SIM-command 88. Card answer <> 9F.',2);
     LWriteln('',2);
     Gsm:=-1;
     Exit;
    end;
    FillChar(WBuffer[0],BufferSize,$0);
    ReadCount:=RBuffer[1];
    RBuffer:=RunCommand(Port,$C0,$00,$00,ReadCount,WBuffer,RError);
    if RError<>0 then begin 
     LWriteln('ERROR! Cannot run SIM-command 88. RunCommand error '+PrintHexByte(RError)+'.',2);
     LWriteln('',2);
     Gsm:=-1;
     Exit;
    end;
    if ReadCount=0 then ReadCount:=256;
    if (RBuffer[ReadCount]<>$90) or (RBuffer[ReadCount+1]<>$00) then begin
     LWriteln('ERROR! Cannot run SIM-command 88. Invalid card answer after C0 command.',2);
     LWriteln('',2);
     Gsm:=-1;
     Exit;
    end;
    Move(RBuffer[0],SRes[0],12);
   end 
   else begin
    if Device=2 then begin
     SRes:=Emu_Comp128v1(KiM,Rand);
    end;
   end;   
   if Res=0 then begin
    AllGSMStep:=AllGSMStep+1;
    if GSMFile<>'' then begin
     Writeln(GsmFo,PrintHexData16(Rand,false)+'-'+PrintHexData12(SRes,false));
     Flush(GsmFo);
    end;
   end;    
   Gsm:=Res;
  end;
 
 {Функция чтения IMSI} 
 function ReadIMSI(var Port:TBlockSerial;var IMSI:String18):Longint; 
  var
   Res:Longint;
   I:Longint;
   ReadCount:Longint;
   RBuffer,WBuffer:TBuffer;
   ImsiLen:Longint;
   S:string;
   RError:byte;
   
  begin
   Res:=0;
   {Select EF_IMSI (6F.07)}
   FillChar(WBuffer[0],BufferSize,$0);
   WBuffer[0]:=$6F;
   WBuffer[1]:=$07;
   RBuffer:=RunCommand(Port,$A4,$00,$00,$02,WBuffer,RError);
   if RError<>0 then begin
    LWriteln('ERROR! Cannot select EF_IMSI. RunCommand error '+PrintHexByte(RError)+'.',2);
    LWriteln('',2);
    ReadIMSI:=-1;
    Exit;
   end;
   if (RBuffer[0]<>$9F) then begin
    LWriteln('ERROR! Cannot select EF_IMSI. Card answer <> 9F.',2);
    LWriteln('',2);
    ReadIMSI:=-1;
    Exit;
   end;
   FillChar(WBuffer[0],BufferSize,$0);
   ReadCount:=RBuffer[1];
   RBuffer:=RunCommand(Port,$C0,$00,$00,ReadCount,WBuffer,RError);
   if RError<>0 then begin
    LWriteln('ERROR! Cannot select EF_IMSI. RunCommand error '+PrintHexByte(RError)+'.',2);
    LWriteln('',2);
    ReadIMSI:=-1;
    Exit;
   end;
   if ReadCount=0 then ReadCount:=256;
   if (RBuffer[ReadCount]<>$90) or (RBuffer[ReadCount+1]<>$00) then begin
    LWriteln('ERROR! Cannot select EF_IMSI. Invalid card answer after C0 command.',2);
    LWriteln('',2);
    ReadIMSI:=-1;
    Exit;
   end;
   {Проверим результат}
   if (RBuffer[4]<>$6F) or (RBuffer[5]<>$07) or (RBuffer[6]<>$04) or (RBuffer[13]<>0) then begin
    LWriteln('ERROR! Invalid EF_IMSI file properties.',2);
    LWriteln('',2);
    ReadIMSI:=-1;
    Exit;
   end;
   ImsiLen:=(Rbuffer[2] shl 8)+RBuffer[3];
   if (ImsiLen<1) or (ImsiLen>256) then begin
    LWriteln('ERROR! Invalid EF_IMSI file length.',2);
    LWriteln('',2);
    ReadIMSI:=-1;
    Exit;
   end;
   if ImsiLen<>9 then begin
    LWriteln('WARNING! EF_IMSI file length not equal 9 bytes.',2);
    LWriteln('',2);
   end;
   {Читаем содержимое файла}
   FillChar(WBuffer[0],BufferSize,$0);
   RBuffer:=RunCommand(Port,$B0,$00,$00,ImsiLen,WBuffer,RError);
   if RError<>0 then begin
    LWriteln('ERROR! Cannot read EF_IMSI. RunCommand error '+PrintHexByte(RError)+'.',2);
    LWriteln('',2);
    ReadIMSI:=-1;
    Exit;
   end;
   ReadCount:=ImsiLen;
   if (RBuffer[ReadCount]<>$90) or (RBuffer[ReadCount+1]<>$00) then begin
    LWriteln('ERROR! Cannot read EF_IMSI. Invalid card answer after B0 command.',2);
    LWriteln('',2);
    ReadIMSI:=-1;
    Exit;
   end;
   {Преобразовываем результат в читаемую форму}
   S:='';
   for I:=0 to ReadCount-1 do S:=S+PrintHexByte(RBuffer[I]);   
   if length(S)>18 then S:=Copy(S,1,18);
   IMSI:=S;
   ReadIMSI:=Res;
  end;


  function migalka(var Port:TBlockSerial;sl1:longInt;sl2:longint):Longint;
  var
   RBuffer,WBuffer:TBuffer;
   RError:byte;

  begin
   RError:=0;
   while (Rerror=0) do begin
    FillChar(WBuffer[0],BufferSize,$0);
    WBuffer[0]:=$6F;
    WBuffer[1]:=$07;
    RBuffer:=RunCommand(Port,$A4,$00,$00,$02,WBuffer,RError);

    Sleep(sl1);

    FillChar(WBuffer[0],BufferSize,$0);
    RBuffer:=RunCommand(Port,$B0,$00,$00,9,WBuffer,RError);

    Sleep(sl2);
   end;
   
   migalka:=RError;
  end;

 {Функция чтения ICCID} 
 function ReadICCID(var Port:TBlockSerial;var ICCID:String19):Longint; 
  var
   Res:Longint;
   I:Longint;
   ReadCount:Longint;
   RBuffer,WBuffer:TBuffer;
   ICCIDLen:Longint;
   S:string;
   RError:byte;
   
  begin
   Res:=0;
   {Select EF_ICCID 2F E2 ??EF_IMSI (7F 20 /6F.07)}




   FillChar(WBuffer[0],BufferSize,$0);
   WBuffer[0]:=$2F;
   WBuffer[1]:=$E2;
   RBuffer:=RunCommand(Port,$A4,$00,$00,$02,WBuffer,RError);
   if RError<>0 then begin
    LWriteln('ERROR! Cannot select EF_ICCID. RunCommand error '+PrintHexByte(RError)+'.',2);
    LWriteln('',2);
    ReadICCID:=-1;
    Exit;
   end;
   if (RBuffer[0]<>$9F) then begin
    LWriteln('ERROR! Cannot select EF_ICCID. Card answer <> 9F.',2);
    LWriteln(PrintHexByte(RBuffer[0]),2);
    LWriteln(PrintHexByte(RBuffer[1]),2);
    LWriteln('',2);
    ReadICCID:=-1;
    Exit;
   end;

   ICCIDLen:=RBuffer[1];

   {Читаем содержимое файла}
   FillChar(WBuffer[0],BufferSize,$0);

   RBuffer:=RunCommand(Port,$B0,$00,$00,$0A,WBuffer,RError);
//   RBuffer:=RunCommand(Port,$B0,$00,$00,ICCIDLen,WBuffer,RError);
   if RError<>0 then begin
    LWriteln('ERROR! Cannot read EF_ICCID. RunCommand error '+PrintHexByte(RError)+'.',2);
    LWriteln('',2);
    ReadICCID:=-1;
    Exit;
   end;
   ReadCount:=10;
   if (RBuffer[ReadCount]<>$90) or (RBuffer[ReadCount+1]<>$00) then begin
    LWriteln('ERROR! Cannot read EF_ICCID. Invalid card answer after B0 command.',2);
    LWriteln('',2);
    LWriteln(PrintHexByte(RBuffer[0]),2);
    LWriteln(PrintHexByte(RBuffer[1]),2);
    ReadICCID:=-1;
    Exit;
   end;
   {Преобразовываем результат в читаемую форму}
   S:='';
   for I:=0 to ReadCount-1 do S:=S+PrintHexByte(RBuffer[I]);   
   //if length(S)>19 then S:=Copy(S,1,19);
   ICCID:=S;
   ReadICCID:=Res;
  end;

 
 {Функция чтения файловой системы СИМ-ки} 
 function ReadSIMInfo(var Port:TBlockSerial;ReadExist:Boolean):Longint; 
  type
   StackRec=record
    CurDir:TPair;
    DirToFind:byte;
    Return:boolean;
   end;
  var
   Res:Longint;
   Stack:array[1..8] of StackRec;
   StackUk,FileUk:byte;
   I:Longint;
   ReadCount:Longint;
   RBuffer,WBuffer:TBuffer;
   RError:byte;
   CurFile:TPair;
   U:Boolean;
   FileMnemonicName:String;
   FileSearch:boolean;
   CurEF:TPair;
   FileToSearch:byte;
   FileLen,CurPos:Word;
   Readed:byte;
   RecLen,RecCount,CurRec,ViewLen:byte;
   PrFirst:boolean;
   
  begin
   Res:=0;
   StackUk:=1;
   Stack[1].CurDir[0]:=$3F;
   Stack[1].CurDir[1]:=$00;
   Stack[1].DirToFind:=1;
   Stack[1].Return:=false;
   FileSearch:=false;
   CurEF[0]:=0;
   CurEF[1]:=0;
   FileToSearch:=0;
   FileUk:=1;
   U:=True;
   While U=true do begin
    if (Stack[StackUk].DirToFind>0) or (FileSearch=true) then begin
     if FileSearch=true then begin
      CurFile:=CurEF;
     end 
     else begin
      CurFile:=Stack[StackUk].CurDir;
     end; 
     FillChar(WBuffer[0],BufferSize,$0);
     Move(CurFile[0],WBuffer[0],2);
     {Выбираем каталог или файл}
     RBuffer:=RunCommand(Port,$A4,$00,$00,$02,WBuffer,RError);
     if RError<>0 then begin
      LWriteln('ERROR! Cannot select '+PrintHexPair(CurFile,false)+'. RunCommand error '+PrintHexByte(RError)+'.',2);
      LWriteln('',2);
      ReadSIMInfo:=-1;
      Exit;
     end;
     if (RBuffer[0]=$9F) then begin
      FillChar(WBuffer[0],BufferSize,$0);
      ReadCount:=RBuffer[1];
      RBuffer:=RunCommand(Port,$C0,$00,$00,ReadCount,WBuffer,RError);
      if RError<>0 then begin
       LWriteln('ERROR! Cannot select '+PrintHexPair(CurFile,false)+'. RunCommand error '+PrintHexByte(RError)+'.',2);
       LWriteln('',2);
       ReadSIMInfo:=-1;
       Exit;
      end;
      if ReadCount=0 then ReadCount:=256;
      if (RBuffer[ReadCount]<>$90) or (RBuffer[ReadCount+1]<>$00) then begin
       LWriteln('ERROR! Cannot select '+PrintHexPair(CurFile,false)+'. Invalid card answer after C0 command.',2);
       LWriteln('',2);
       ReadSIMInfo:=-1;
       Exit;
      end;
      if (Stack[StackUk].Return=false) or (FileSearch=true) then begin
       FileMnemonicName:='';
       for I:=1 to FileIdCount do begin
        if (FileId[I][0]=CurFile[0]) and (FileId[I][1]=CurFile[1]) then begin
         FileMnemonicName:=FileName[I];
         break;
        end;
	if (FileId[I][0]>=CurFile[0]) and (FileId[I][1]>CurFile[1]) then break;
       end;
       if FileMnemonicName='' then FileMnemonicName:=PrintHexPair(CurFile,false)+'      ';
       {Выводим информацию}
       for I:=1 to (FileUk-1) do LWrite(' ',2);
       if (CurFile[0]=$3F) or (CurFile[0]=$7F) then LWrite('D - ',2) else LWrite('F - ',2);
       LWrite(FileMnemonicName+' ',2);
       if FileSearch=false then begin
        LWrite('dirs='+IntToStr(RBuffer[14])+',',2);
        LWrite('files='+IntToStr(RBuffer[15]),2);
       end
       else begin
        LWrite('s='+PrintHexByte(RBuffer[2])+PrintHexByte(RBuffer[3])+',',2);
	LWrite('r=[',2);
	LWrite('RD='+RightsMas[(RBuffer[8] and $F0) shr 4]+' ',2);
	LWrite('UPD='+RightsMas[RBuffer[8] and $0F]+' ',2);
	LWrite('INC='+RightsMas[(RBuffer[9] and $F0) shr 4]+' ',2);
	LWrite('REH='+RightsMas[(RBuffer[10] and $F0) shr 4]+' ',2);
        LWrite('INV='+RightsMas[RBuffer[10] and $0F]+'],',2);
	LWrite('s=',2);
	case RBuffer[13] of
	 0:LWrite('trn',2);
	 1:LWrite('lin('+PrintHexByte(RBuffer[14])+')',2);
	 3:LWrite('cyc('+PrintHexByte(RBuffer[14])+')',2);
	 else LWrite('unk',2);
	end;
       end;
       LWriteln('',2);
       {Если нужно читать содержимое файлов (--read) - считаем}
       if (FileSearch=true) and (ReadExist=true) then begin
        case RBuffer[13] of
	 0:begin
	  {Обычный файл - набор данных}
	  FileLen:=(RBuffer[2] shl 8)+RBuffer[3];
	  if FileLen>0 then begin
	   FillChar(WBuffer[0],BufferSize,$0);
	   CurPos:=0;
	   while FileLen>0 do begin
	    if FileLen>$10 then begin
	     {Читаем по 16 байтов}
	     Readed:=$10;
	     RBuffer:=RunCommand(Port,$B0,(CurPos shr 8),(CurPos and $00FF),Readed,WBuffer,RError);
	     FileLen:=FileLen-Readed;
	     CurPos:=CurPos+Readed;
	    end
	    else begin
	     Readed:=FileLen;
	     RBuffer:=RunCommand(Port,$B0,(CurPos shr 8),(CurPos and $00FF),Readed,WBuffer,RError);
	     CurPos:=CurPos+FileLen;
	     FileLen:=0;
	    end;
	    if RError<>0 then begin
             LWriteln('ERROR! Cannot read from file '+PrintHexPair(CurFile,false)+'. RunCommand error '+PrintHexByte(RError)+'.',2);
             LWriteln('',2);
             ReadSIMInfo:=-1;
             Exit;
	    end 
	    else begin
	     {Выведем результат чтения}
  	     for I:=1 to FileUk+1 do LWrite(' ',2);
	     {Шестнадцатеричный вид}
	     for I:=0 to Readed-1 do begin
	      LWrite(PrintHexByte(RBuffer[I])+' ',2);
	     end;
	     if Readed<$10 then begin
 	      for I:=Readed+1 to $10 do begin
	       LWrite('   ',2);
	      end;
	     end;
	     {Символьный вид}
	     LWrite('(',2);
	     for I:=0 to Readed-1 do begin
	      if RBuffer[I]>=32 then begin
	       LWrite(Chr(RBuffer[I]),2);
	      end
	      else begin
	       LWrite('.',2);
	      end;
	     end;
	     if Readed<$10 then begin
	      for I:=Readed+1 to $10 do begin
	       LWrite(' ',2);
	      end;
	     end;
	     LWrite(')',2);
	     LWriteLn('',2);
	    end;
	   end;    
	  end;     
	 end;
	 1,3:begin
	  {Линейный или циклический файл - файл с записями}
	  FileLen:=(RBuffer[2] shl 8)+RBuffer[3];
	  RecLen:=RBuffer[14];
	  if (FileLen<>0) and (RecLen<>0) then begin
	   FillChar(WBuffer[0],BufferSize,$0);
	   RecCount:=FileLen div RecLen;
	   for CurRec:=1 to RecCount do begin
	    {Читаем одну запись}
	    RBuffer:=RunCommand(Port,$B2,CurRec,$04,RecLen,WBuffer,RError);
	    if RError<>0 then begin
             LWriteln('ERROR! Cannot read from file '+PrintHexPair(CurFile,false)+'. RunCommand error '+PrintHexByte(RError)+'.',2);
             LWriteln('',2);
             ReadSIMInfo:=-1;
             Exit;
	    end 
	    else begin
	     {Выведем результат чтения}
	     PrFirst:=true;
	     ViewLen:=RecLen;
	     while ViewLen>0 do begin
	      if ViewLen>$10 then Readed:=$10 else Readed:=ViewLen;
   	      for I:=1 to FileUk do LWrite(' ',2);
	      if PrFirst then begin
 	       LWrite('*',2);
	       PrFirst:=false;
	      end
	      else begin
 	       LWrite(' ',2);
	      end; 
	      {Шестнадцатеричный вид}
 	      for I:=0 to Readed-1 do begin
	       LWrite(PrintHexByte(RBuffer[RecLen-ViewLen+I])+' ',2);
	      end;
	      if Readed<$10 then begin
 	       for I:=Readed+1 to $10 do begin
	        LWrite('   ',2);
	       end;
	      end;
	      {Символьный вид}
	      LWrite('(',2);
	      for I:=0 to Readed-1 do begin
	       if RBuffer[RecLen-ViewLen+I]>=32 then begin
	        LWrite(Chr(RBuffer[RecLen-ViewLen+I]),2);
	       end
	       else begin
	        LWrite('.',2);
	       end;
	      end;
	      if Readed<$10 then begin
	       for I:=Readed+1 to $10 do begin
	        LWrite(' ',2);
	       end;
	      end;
	      LWrite(')',2);
	      ViewLen:=ViewLen-Readed;
	      LWriteLn('',2);
	     end;
            end;	    
	   end;
	  end;
	 end
	 else begin
	  for I:=1 to FileUk do LWrite(' ',2);
	  LWriteln('<Unknown file structure!>',2);
	 end;
	end;
       end;
       {Перейдем к следующему файлу}
       if FileSearch=true then begin
        FileToSearch:=FileToSearch-1;
	if FileToSearch>0 then begin
         if CurEF[1]<$FF then begin
          CurEF[1]:=CurEF[1]+1;
         end 
         else begin
          FileToSearch:=0;
  	  FileSearch:=false;
         end;
	end
	else FileSearch:=False; 
       end
       else begin
        {Отметка о каталоге}
	FileUk:=FileUk+1;
        if RBuffer[15]<>0 then begin
         FileSearch:=true;
 	 FileToSearch:=RBuffer[15];
 	 if StackUk=1 then CurEF[0]:=$2F else CurEF[0]:=$6F;
	 CurEF[1]:=0;
        end;
        Stack[StackUk].DirToFind:=Stack[StackUk].DirToFind-1;
        Stack[StackUk].Return:=true;
        if RBuffer[14]<>0 then begin
         StackUk:=StackUk+1;
         Stack[StackUk].DirToFind:=RBuffer[14];
         Stack[StackUk].CurDir[0]:=$7F;
         Stack[StackUk].CurDir[1]:=$00;
         Stack[StackUk].Return:=false;
        end;
       end;	
      end
      else begin
       FileUk:=FileUk-1;
       Stack[StackUk].Return:=false;
       if Stack[StackUk].CurDir[1]<$FF then begin
        Stack[StackUk].CurDir[1]:=Stack[StackUk].CurDir[1]+1;
       end
       else begin
        Stack[StackUk].DirToFind:=0;
       end; 
      end; 
     end
     else begin
      {Карта ответила не 9F}
      if FileSearch=true then begin
       if CurEF[1]<$FF then begin
        CurEF[1]:=CurEF[1]+1;
       end 
       else begin
        FileToSearch:=0;
	FileSearch:=false;
       end;
      end 
      else begin
       if Stack[StackUk].Return=false then begin
        if Stack[StackUk].CurDir[1]<$FF then begin
         Stack[StackUk].CurDir[1]:=Stack[StackUk].CurDir[1]+1;
        end
        else begin
         Stack[StackUk].DirToFind:=0;
        end; 
       end
       else begin
        LWriteln('ERROR! Cannot select '+PrintHexPair(CurFile,false)+'. Card answer <> 9F.',2);
        LWriteln('',2);
        ReadSIMInfo:=-1;
        Exit;
       end;
      end; 
     end; 
    end
    else begin
     StackUk:=StackUk-1;
     FileUk:=FileUk-1;
     if StackUk=0 then U:=False;
    end; 
   end; 
   LWriteln('',2);
   ReadSIMInfo:=Res;
  end;  

 {Функция показа СМС-ок из СИМ-ки}   
 function ViewSMS(var Port:TBlockSerial):Longint; 
  var
   Res:Longint;
   ReadCount:Longint;
   RBuffer,WBuffer:TBuffer;
   RError:byte;
   FileLen:Word;
   RecLen,RecCount,CurRec:Byte;
   SMSFound:boolean;
   S:string;
   SMSC,OA:TAddress;
   UkInSMS:byte;
   PrOk:boolean;
   TP_MTI:byte;
   UDHI:byte;
   Direct:byte;
   SMSLen:byte;
   TSCAddr:word;
   AddrLen:byte;
   TSC:TTSC;
   Alpha,Comp:byte;
   UDL,UDLB:byte;
   UDH:TBody;
   UDHL,UDHL7:byte;
   DeltaBits:byte;
   Body:TBody;

  begin
   Res:=0;
   SMSFound:=false;
   {Select DF_TELECOM (7F.10)}
   FillChar(WBuffer[0],BufferSize,$0);
   WBuffer[0]:=$7F;
   WBuffer[1]:=$10;
   RBuffer:=RunCommand(Port,$A4,$00,$00,$02,WBuffer,RError);
   if RError<>0 then begin
    LWriteln('ERROR! Cannot select DF_TELECOM. RunCommand error '+PrintHexByte(RError)+'.',2);
    LWriteln('',2);
    ViewSMS:=-1;
    Exit;
   end;
   if (RBuffer[0]<>$9F) then begin
    LWriteln('ERROR! Cannot select DF_TELECOM. Card answer <> 9F.',2);
    LWriteln('',2);
    ViewSMS:=-1;
    Exit;
   end;
   FillChar(WBuffer[0],BufferSize,$0);
   ReadCount:=RBuffer[1];
   RBuffer:=RunCommand(Port,$C0,$00,$00,ReadCount,WBuffer,RError);
   if RError<>0 then begin
    LWriteln('ERROR! Cannot select DF_TELECOM. RunCommand error '+PrintHexByte(RError)+'.',2);
    LWriteln('',2);
    ViewSMS:=-1;
    Exit;
   end;
   if ReadCount=0 then ReadCount:=256;
   if (RBuffer[ReadCount]<>$90) or (RBuffer[ReadCount+1]<>$00) then begin
    LWriteln('ERROR! Cannot select DF_TELECOM. Invalid card answer after C0 command.',2);
    LWriteln('',2);
    ViewSMS:=-1;
    Exit;
   end;
   {Select EF_SMS (6F.3C)}
   FillChar(WBuffer[0],BufferSize,$0);
   WBuffer[0]:=$6F;
   WBuffer[1]:=$3C;
   RBuffer:=RunCommand(Port,$A4,$00,$00,$02,WBuffer,RError);
   if RError<>0 then begin
    LWriteln('ERROR! Cannot select EF_SMS. RunCommand error '+PrintHexByte(RError)+'.',2);
    LWriteln('',2);
    ViewSMS:=-1;
    Exit;
   end;
   if (RBuffer[0]<>$9F) then begin
    LWriteln('ERROR! Cannot select EF_SMS. Card answer <> 9F.',2);
    LWriteln('',2);
    ViewSMS:=-1;
    Exit;
   end;
   FillChar(WBuffer[0],BufferSize,$0);
   ReadCount:=RBuffer[1];
   RBuffer:=RunCommand(Port,$C0,$00,$00,ReadCount,WBuffer,RError);
   if RError<>0 then begin
    LWriteln('ERROR! Cannot select EF_SMS. RunCommand error '+PrintHexByte(RError)+'.',2);
    LWriteln('',2);
    ViewSMS:=-1;
    Exit;
   end;
   if ReadCount=0 then ReadCount:=256;
   if (RBuffer[ReadCount]<>$90) or (RBuffer[ReadCount+1]<>$00) then begin
    LWriteln('ERROR! Cannot select EF_SMS. Invalid card answer after C0 command.',2);
    LWriteln('',2);
    ViewSMS:=-1;
    Exit;
   end;
   {Читаем записи}
   FileLen:=(RBuffer[2] shl 8)+RBuffer[3];
   RecLen:=RBuffer[14];
   if (FileLen<>0) and (RecLen<>0) then begin
    FillChar(WBuffer[0],BufferSize,$0);
    RecCount:=FileLen div RecLen;
    for CurRec:=1 to RecCount do begin
     {Читаем одну запись}
     RBuffer:=RunCommand(Port,$B2,CurRec,$04,RecLen,WBuffer,RError);
     if RError<>0 then begin
      LWriteln('ERROR! Cannot read from file EF_SMS. RunCommand error '+PrintHexByte(RError)+'.',2);
      LWriteln('',2);
      ViewSMS:=-1;
      Exit;
     end 
     else begin
      {Разберем результат чтения}
      if RBuffer[1]<>$FF then begin
       {СМС-ка не пустая}
       SMSFound:=true;
       UkInSMS:=0;
       PrOk:=true;
       Alpha:=0;
       Comp:=0;
       LWriteln('--------------------------',2);
       LWrite('NUM='+IntToStr(CurRec)+' ',2);
       LWrite('ID='+PrintHexByte(RBuffer[UkInSMS])+' ',2);
       LWrite('SMS-C=',2);
       if (RBuffer[UkInSMS+1]>$0B) or (RBuffer[UkInSMS+1]=$01) then begin
        {Неправильная длина SMS-C адреса}
	LWriteln('"ERROR! Invalid length ('+PrintHexByte(RBuffer[UkInSMS+1])+')!"',2);
	PrOk:=false;
       end
       else begin
        SMSC.SLen:=(RBuffer[UkInSMS+1]*8) div 7;
        Move(RBuffer[UkInSMS+1],SMSC.Len,RBuffer[UkInSMS+1]+1);
	S:=UnpackAddress(SMSC);
	LWriteln(CharsetUpdate(S),2);
	UkInSMS:=RBuffer[UkInSMS+1]+2;
       end;
       LWriteln('--------------------------',2);
       if PrOK then begin
        {Если удалось разобрать SMS-C, то продолжаем}
	Direct:=0;
	TP_MTI:=RBuffer[UkInSMS] and $03;
	LWrite('MTI=',2);
	{Определим длину SMS-ки}
	SMSLen:=RecLen-1;
	while RBuffer[SMSLen]=$FF do SMSLen:=SMSLen-1;
	SMSLen:=SMSLen-UkInSMS+1;
	if SMSLen<2 then SMSLen:=2;
	{Определим тип сообщения и направление передачи}
	case TP_MTI of
	 0:begin
	  if (SMSLen>=2) and (SMSLen<=12) then begin
	   Direct:=2;
	  end
	  else begin
	   AddrLen:=RBuffer[UkInSMS+1];
	   if AddrLen>$14 then begin
	    Direct:=2;
	   end
	   else begin
	    if AddrLen=0 then TSCAddr:=0 else TSCAddr:=((AddrLen-1) shr 1)+1;
	    TSCAddr:=TSCAddr+UkInSMS+5;
	    Move(RBuffer[TSCAddr],TSC,7);
	    if IsTSCCorrect(TSC) then begin
	     Direct:=1;
	    end
	    else begin
	     Direct:=2;
	    end;
	   end;
	  end;
	  if Direct=1 then begin
 	   LWriteln('SMS-DELIVER',2);
	  end
	  else begin
	   LWriteln('SMS-DELIVER-REPORT',2);
	  end;
	 end;
	 1:begin
	 
	 end;
	 2:begin
	 
	 end;
	 3:begin
	  TP_MTI:=0;
	  Direct:=1;
	  LWriteln('SMS-DELIVER (Reserved)',2);
	 end;
	end;
	{Выведем информацию из заголовка и саму СМС-ку}
	if Direct=0 then begin
	 LWriteln('"ERROR! Cannot determine message type and direction!"',2);
	end
	else begin
 	 case TP_MTI of
	  0:begin
	   if Direct=1 then begin
	    {SMS-DELIVER}
	    {Выведем битовые поля из первого байта}
	    S:=UnpackMTIByteData(RBuffer[UkInSMS],Direct,UDHI);
	    LWriteln(S,2);
	    UkInSMS:=UkInSMS+1;
	    {Разберем и выведем OA - оригинальный адрес}
	    LWrite('SENDER=',2);
	    AddrLen:=RBuffer[UkInSMS];
	    if AddrLen>$14 then begin
             {Неправильная длина OA адреса}
	     LWriteln('"ERROR! Invalid length ('+PrintHexByte(AddrLen)+')!"',2);
	     PrOk:=false;
	    end
	    else begin
	     OA.SLen:=(AddrLen*4) div 7;
	     if AddrLen>0 then AddrLen:=((AddrLen-1) shr 1)+2;
	     Move(RBuffer[UkInSMS],OA.Len,AddrLen+1);
	     OA.Len:=AddrLen;
  	     S:=UnpackAddress(OA);
	     LWriteln(CharsetUpdate(S),2);
	     if AddrLen=0 then UkInSMS:=UkInSMS+2 else UkInSMS:=UkInSMS+AddrLen+1;
	    end;    
	    if PrOK then begin
	     {Выведем PID}
	     LWrite('PID:(',2);
	     S:=UnpackPID(Rbuffer[UkInSMS]);
	     LWrite(S,2);
	     LWriteln(')',2);
	     UkInSMS:=UkInSMS+1;
	     {Разберем и выведем DCS}
	     LWrite('DCS:(',2);
	     S:=UnpackDCS(RBuffer[UkInSMS],Alpha,Comp);
	     LWrite(S,2);
	     LWriteln(')',2);
	     UkInSMS:=UkInSMS+1;
	     {Выведем TSC}
	     Move(RBuffer[UkInSMS],TSC,7);
	     LWrite('TSC=',2);
	     S:=UnpackTSC(TSC);
	     LWriteln(S,2);
	     UkInSMS:=UkInSMS+7;
	     {Считаем UDL}
	     UDL:=RBuffer[UkInSMS];
	     UDLB:=UDL;
	     UkInSMS:=UkInSMS+1;
	    end;
	   end
	   else begin
	    {SMS-DELIVER-REPORT}
	   end;
	  end;
	  1:begin
	   if Direct=1 then begin
	    {SMS-SUBMIT-REPORT}
	   end
	   else begin
	    {SMS-SUBMIT}
	   end;
	  end;
	  2:begin
	   if Direct=1 then begin
	    {SMS-STATUS-REPORT}
	   end
	   else begin
	    {SMS-COMMAND}
	   end;
	  end;
	 end;
	 S:='';
	 if PrOk then begin
	  if UDL=0 then begin
	   LWriteln('[<No SMS-body!!!>]',2);
	  end
	  else begin
	   if Alpha=3 then begin
	    LWriteln('[<Reserved alphabet! Cannot decode body!>]',2);
	   end
	   else begin
 	    if (Comp=0) and (Alpha=0) then begin
	     UDLB:=(((UDL*7)-1) div 8)+1;
	    end;
 	    {Выведем UDH}
	    if UDHI=1 then begin
	     UDHL:=RBuffer[UkInSMS];
	     UkInSMS:=UkInSMS+1;
	     LWrite('Len='+PrintHexByte(UDHL)+' ',2);
	     Move(RBuffer[UkInSMS],UDH[0],UDHL);
	     UkInSMS:=UkInSMS+UDHL;
	     LWriteln('UDH:('+UnpackUDH(UDH,UDHL)+')',2);
	     UDHL:=UDHL+1;
	     UDLB:=UDLB-UDHL;
	     if (Comp=0) and (Alpha=0) then begin
	      UDHL7:=(((UDHL*8)-1) div 7)+1;
	      DeltaBits:=7-((UDHL*8) mod 7);
	      if DeltaBits=7 then DeltaBits:=0;
	      UDL:=UDL-UDHL7;
	     end
	     else begin
	      UDL:=UDL-UDHL;
	     end;
	    end;
	    {Выведем текст сообщения}
	    LWrite('[',2);
	    if (UDLB>0) and (UDL>0) then begin
	     Move(RBuffer[UkInSMS],Body[0],UDLB);   
	     if Comp=0 then begin
	      case Alpha of 
	       0:begin
	        S:=UnpackAlpha(Body,UDLB,UDL);
	       end;
	       1:begin
	       end;
	       2:begin
	        S:=UnpackUCS2(Body,UDLB);
	       end;
	      end;
	     end
	     else begin
	    
	     end;
	     LWrite(CharsetUpdate(S),2);
	    end
	    else LWrite('<No SMS-body!!!>',2);
	    LWriteln(']',2);
	   end;
	  end;
	 end;
	end;
       end;
       LWriteln('',2);
      end;
     end;
    end;
   end;       
   if SMSFound=false then begin
    LWriteln('No SMS found...',2);
    LWriteln('',2);
   end;
   ViewSMS:=Res;
  end;

 {Функция показа ADN/FDN-ок из СИМ-ки}   
 function ViewADN(var Port:TBlockSerial):Longint;
  const 
   ReadFiles:array[1..3] of TPair=(($6F,$3A),($6F,$3B),($6F,$44));
   NameFiles:array[1..3] of String=('EF_ADN','EF_FDN','EF_LND');
  var
   Res:Longint;
   ReadCount:Longint;
   RBuffer,WBuffer:TBuffer;
   RError:byte;
   CurFile:byte;
   FileLen:Word;
   RecLen,RecCount,CurRec:Byte;
   PrFound:boolean;
   ClnAddr:TAddress;
   AddrLen:byte;
   S:string;
   Body:TBody;
   BodyLen:byte;

  begin
   Res:=0;
   {Select DF_TELECOM (7F.10)}
   FillChar(WBuffer[0],BufferSize,$0);
   WBuffer[0]:=$7F;
   WBuffer[1]:=$10;
   RBuffer:=RunCommand(Port,$A4,$00,$00,$02,WBuffer,RError);
   if RError<>0 then begin
    LWriteln('ERROR! Cannot select DF_TELECOM. RunCommand error '+PrintHexByte(RError)+'.',2);
    LWriteln('',2);
    ViewADN:=-1;
    Exit;
   end;
   if (RBuffer[0]<>$9F) then begin
    LWriteln('ERROR! Cannot select DF_TELECOM. Card answer <> 9F.',2);
    LWriteln('',2);
    ViewADN:=-1;
    Exit;
   end;
   FillChar(WBuffer[0],BufferSize,$0);
   ReadCount:=RBuffer[1];
   RBuffer:=RunCommand(Port,$C0,$00,$00,ReadCount,WBuffer,RError);
   if RError<>0 then begin
    LWriteln('ERROR! Cannot select DF_TELECOM. RunCommand error '+PrintHexByte(RError)+'.',2);
    LWriteln('',2);
    ViewADN:=-1;
    Exit;
   end;
   if ReadCount=0 then ReadCount:=256;
   if (RBuffer[ReadCount]<>$90) or (RBuffer[ReadCount+1]<>$00) then begin
    LWriteln('ERROR! Cannot select DF_TELECOM. Invalid card answer after C0 command.',2);
    LWriteln('',2);
    ViewADN:=-1;
    Exit;
   end;
   {Цикл по файлам}
   for CurFile:=1 to 3 do begin
    {Select current file (6F.xx)}
    LWriteln('------------ '+NameFiles[CurFile]+' ------------',2);
    FillChar(WBuffer[0],BufferSize,$0);
    WBuffer[0]:=ReadFiles[CurFile,0];
    WBuffer[1]:=ReadFiles[CurFile,1];
    RBuffer:=RunCommand(Port,$A4,$00,$00,$02,WBuffer,RError);
    if RError<>0 then begin
     LWriteln('ERROR! Cannot select '+NameFiles[CurFile]+'. RunCommand error '+PrintHexByte(RError)+'.',2);
     LWriteln('',2);
     ViewADN:=-1;
     Exit;
    end;
    if (RBuffer[0]<>$9F) then begin
     LWriteln('ERROR! Cannot select '+NameFiles[CurFile]+'. Card answer <> 9F.',2);
     LWriteln('',2);
     ViewADN:=-1;
     Exit;
    end;
    FillChar(WBuffer[0],BufferSize,$0);
    ReadCount:=RBuffer[1];
    RBuffer:=RunCommand(Port,$C0,$00,$00,ReadCount,WBuffer,RError);
    if RError<>0 then begin
     LWriteln('ERROR! Cannot select '+NameFiles[CurFile]+'. RunCommand error '+PrintHexByte(RError)+'.',2);
     LWriteln('',2);
     ViewADN:=-1;
     Exit;
    end;
    if ReadCount=0 then ReadCount:=256;
    if (RBuffer[ReadCount]<>$90) or (RBuffer[ReadCount+1]<>$00) then begin
     LWriteln('ERROR! Cannot select '+NameFiles[CurFile]+'. Invalid card answer after C0 command.',2);
     LWriteln('',2);
     ViewADN:=-1;
     Exit;
    end;
    {Читаем записи}
    PrFound:=false;
    FileLen:=(RBuffer[2] shl 8)+RBuffer[3];
    RecLen:=RBuffer[14];
    if (FileLen<>0) and (RecLen<>0) then begin
     FillChar(WBuffer[0],BufferSize,$0);
     RecCount:=FileLen div RecLen;
     for CurRec:=1 to RecCount do begin
      {Читаем одну запись}
      RBuffer:=RunCommand(Port,$B2,CurRec,$04,RecLen,WBuffer,RError);
      if RError<>0 then begin
       LWriteln('ERROR! Cannot read from file '+NameFiles[CurFile]+'. RunCommand error '+PrintHexByte(RError)+'.',2);
       LWriteln('',2);
       ViewADN:=-1;
       Exit;
      end 
      else begin
       {Разберем результат чтения}
       if RBuffer[RecLen-14]<>$FF then begin
        PrFound:=true;
	S:=IntToStr(CurRec);
	while length(S)<3 do S:=' '+S;
	LWrite(S+'): ',2);
	AddrLen:=RBuffer[RecLen-14];
	if (AddrLen>$0B) or (AddrLen=$01) then begin
         LWriteln('"ERROR! Invalid address length ('+PrintHexByte(AddrLen)+')!"',2);	
	end
	else begin
	 BodyLen:=RecLen-14;
	 while (BodyLen>0) and (RBuffer[BodyLen-1]=$FF) do BodyLen:=BodyLen-1;
	 if BodyLen=0 then begin
	  S:='<No body!!!>';
	  while length(S)<24 do S:=S+' ';
	  LWrite(S+' - ',2)
	 end
	 else begin
	  if RBuffer[0]=$80 then begin
	   {UCS-2}
	   BodyLen:=BodyLen-1;
	   Move(RBuffer[1],Body[0],BodyLen);
	   S:=UnpackUCS2(Body,BodyLen);
	  end
	  else begin
	   {8bit}
	   Move(RBuffer[0],Body[0],BodyLen);
	   S:=Unpack8bit(Body,BodyLen);
	  end; 
	  if length(S)<24 then begin
	   while length(S)<24 do S:=S+' ';
	  end;
	  LWrite(CharsetUpdate(S)+' - ',2);
	 end;
         ClnAddr.SLen:=(AddrLen*8) div 7;
         Move(RBuffer[RecLen-14],ClnAddr.Len,AddrLen+1);
  	 S:=UnpackAddress(ClnAddr);
	 LWriteln(CharsetUpdate(S),2);
	end;
       end;
      end;
     end;
     if PrFound=false then LWriteln('No data found...',2);
     LWriteln('',2);   
    end;  
   end;
   ViewADN:=Res;
  end;
 
 {Функция чтения сервисного лога телефона} 
 function ReadInitLog(var Port:TBlockSerial;Seconds:Longword):Longint; 
  var
   U:boolean;
   Res:Longint;
   B:byte;
   StartTime,CurTime,AllTime:Longint;
  begin
   Res:=0;
   if Seconds<>0 then StartTime:=GetTick;
   U:=true;
   while U=true do begin
    if Port.CanReadEx(READ_CONST_TIMEOUT) then begin
     B:=Port.RecvByte(READ_TIMEOUT);
     if (B<>0) and (B<>$0D) then LWrite(Chr(B),2);
    end 
    else begin
     if Seconds<>0 then begin
      CurTime:=GetTick;
      AllTime:=TickDelta(StartTime,CurTime) div 1000;
      if AllTime>=Seconds then U:=false;
     end;     
    end; 
   end;
   ReadInitLog:=Res;
  end;
  
 {Функция проведения Boot Authentification для R520m} 
 function BootAuth(var Port:TBlockSerial;Timeout:Longint;Applet:string):Longint; 
  var
   Res:Longint;
   U:boolean;
   B:byte;
   WaitCount:Longint;
   I,Uk:Longint;
   BufferString:string;
   Buffer:TBuffer;
   BufferApplet:PApplet;
   AppletSize:Longint;
   AFi:File;
   ReadC:Longint;
  begin
   Res:=0;
   {Загрузим апплет}
   BufferApplet:=nil;
   if Applet='' then begin
    {Internal applet}
    AppletSize:=InternalAppletSize;
    GetMem(BufferApplet,AppletSize);
    if BufferApplet=nil then begin
     LWriteln('ERROR! Cannot allocate memory for BufferApplet!',2);
     BootAuth:=-1;
     exit;
    end;
    Move(InternalApplet[0],BufferApplet^[0],AppletSize);
    LWriteln('Using applet - INTERNAL. Size='+IntToStr(AppletSize),2);
    LWriteln('',2);
   end
   else begin
    {External applet}
    Assign(AFi,Applet);
    {$I-}
    Reset(AFi,1);
    {$I+}
    if IOResult<>0 then begin
     LWriteln('ERROR! Cannot open applet file...',2);
     BootAuth:=-1;
     exit;
    end;
    AppletSize:=FileSize(AFi);
    GetMem(BufferApplet,AppletSize);
    if BufferApplet=nil then begin
     LWriteln('ERROR! Cannot allocate memory for BufferApplet!',2);
     BootAuth:=-1;
     exit;
    end;
    BlockRead(AFi,BufferApplet^[0],AppletSize,ReadC);
    if ReadC<>AppletSize then begin
     LWriteln('ERROR! Error while reading applet file...',2);
     BootAuth:=-1;
     {$I-}
     Close(AFi);
     {$I+}
     If IOResult<>0 then ;
     exit;
    end;
    LWriteln('Using applet - '+Applet+'. Size='+IntToStr(AppletSize),2);
    LWriteln('',2);
    {$I-}
    Close(AFi);
    {$I+}
    If IOResult<>0 then ;
   end;
   {Ждем получения ID платформы}
   B:=0;
   if Timeout<>0 then WaitCount:=0;
   U:=true;
   while U=true do begin
    if Port.CanReadEx(READ_CONST_TIMEOUT) then begin
     B:=Port.RecvByte(READ_TIMEOUT);
     if B=$5A then U:=false;
    end 
    else begin
     if Timeout<>0 then begin
      WaitCount:=WaitCount+READ_CONST_TIMEOUT;
      if WaitCount>=(Timeout*1000) then begin
       U:=false;
       LWriteln('ERROR! Timeout. No answer from phone...',2);
       Res:=1;
      end;
     end; 
    end;
   end;
   if Res=0 then begin
    LWriteln('> Platform ID: '+PrintHexByte(B),2);
    {Пошлем запрос Martha ID}
    Sleep(100);
    Port.SendByte($3F);
    Sleep(100);
    if Port.LastError<>0 then begin
     LWriteln('ERROR! Cannot send Martha ID request to phone!',2);
     BootAuth:=-1;
     Exit;
    end
    else LWriteln('< Martha rqst: '+PrintHexByte($3F),2);
    {Прочитаем Martha ID}
    BufferString:='';
    BufferString:=Port.RecvPacket(READ_CONST_TIMEOUT);
    LWrite('> Martha ID  : ',2);
    Uk:=Length(BufferString);
    if Uk>0 then begin
     for I:=1 to Uk do begin
      LWrite(PrintHexByte(Ord(BufferString[I])),2);
      if I<>Uk then LWrite(' ',2);
     end;
    end
    else LWrite('<No answer!>',2); 
    LWriteln('',2); 
    if (Uk<>8) or (BufferString<>MarthaID_R520m) then begin
     LWriteln('ERROR! Invalid Martha ID...',2);
     BootAuth:=-1;
     Exit;
    end;
    {Пошлем запрос BA}
    Sleep(100);
    Port.SendByte($50);
    Port.SendByte($00);
    Port.SendByte($AB);
    Sleep(100);
    if Port.LastError<>0 then begin
     LWriteln('ERROR! Cannot send Boot Authentification request!',2);
     BootAuth:=-1;
     Exit;
    end
    else LWriteln('< BA request : 50 00 AB',2);
    {Прочитаем ответ}
    BufferString:='';
    BufferString:=Port.RecvPacket(READ_CONST_TIMEOUT);
    LWrite('> Ready to BA: ',2);
    Uk:=Length(BufferString);
    if Uk>0 then begin
     for I:=1 to Uk do begin
      LWrite(PrintHexByte(Ord(BufferString[I])),2);
      if I<>Uk then LWrite(' ',2);
     end;
    end
    else LWrite('<No answer!>',2); 
    LWriteln('',2);
    if (Uk<>3) or (BufferString<>(Chr($45)+Chr($70)+Chr($46))) then begin
     LWriteln('ERROR! Phone not ready to boot authentification...',2);
     BootAuth:=-1;
     Exit;
    end;
    {Пошлем первый блок BA}
    Sleep(100);
    Port.SendByte($42);
    Port.SendByte($00);
    Port.SendByte($01);
    for I:=1 to 64 do Port.SendByte($00);
    Port.SendByte($B8);
    Sleep(100);
    if Port.LastError<>0 then begin
     LWriteln('ERROR! Cannot send Boot Authentification request!',2);
     BootAuth:=-1;
     Exit;
    end
    else LWriteln('< First block: 42 00 01 00....00 B8',2);
    {Прочитаем ответ}
    BufferString:='';
    BufferString:=Port.RecvPacket(READ_CONST_TIMEOUT);
    LWrite('> BA Code    : ',2);
    Uk:=Length(BufferString);
    if Uk>0 then begin
     for I:=1 to Uk do begin
      LWrite(PrintHexByte(Ord(BufferString[I])),2);
      if I<>Uk then LWrite(' ',2);
     end;
    end
    else LWrite('<No answer!>',2); 
    LWriteln('',2);
    if (Uk<>7) or (Copy(BufferString,1,3)<>(Chr($4F)+Chr($00)+Chr($01))) then begin
     LWriteln('ERROR! Phone answer with BA Code is invalid...',2);
     BootAuth:=-1;
     Exit;
    end;
    {Пошлем второй блок BA}
    Sleep(100);
    Port.SendByte($43);
    Port.SendByte($00);
    Port.SendByte($01);
    for I:=1 to 64 do Port.SendByte($00);
    Port.SendByte($B7);
    Sleep(100);
    if Port.LastError<>0 then begin
     LWriteln('ERROR! Cannot send Boot Authentification answer!',2);
     BootAuth:=-1;
     Exit;
    end
    else LWriteln('< Second blck: 43 00 01 00....00 B7',2);
    {Прочитаем ответ}
    BufferString:='';
    BufferString:=Port.RecvPacket(READ_CONST_TIMEOUT);
    LWrite('> Answer code: ',2);
    Uk:=Length(BufferString);
    if Uk>0 then begin
     for I:=1 to Uk do begin
      LWrite(PrintHexByte(Ord(BufferString[I])),2);
      if I<>Uk then LWrite(' ',2);
     end;
    end
    else LWrite('<No answer!>',2); 
    LWriteln('',2);
    if (Uk<>3) or (BufferString<>(Chr($45)+Chr($24)+Chr($92))) then begin
     LWriteln('ERROR! Access denited!',2);
     BootAuth:=-1;
     Exit;
    end;
    LWriteln('Access GRANTED!',2);
    {Загрузим апплет}
    Sleep(100);
    Port.SendByte($58);
    For I:=0 to AppletSize-1 do begin
     Port.SendByte(BufferApplet^[I]);
    end;
    Sleep(100);
    if Port.LastError<>0 then begin
     LWriteln('ERROR! Cannot loading applet!',2);
     BootAuth:=-1;
     Exit;
    end
    else LWriteln('< Load applet: 58 Applet',2);
    {Прочитаем приглашение апплета}
    Sleep(3000);
    Uk:=ReadCard(Port,Buffer,0);
    LWrite('> Applet cmd : ',2);
    if Uk>0 then begin
     for I:=0 to Uk-1 do begin
      LWrite(PrintHexByte(Buffer[I])+' ',2);
     end;
     LWrite('(',2);
     for I:=0 to Uk-1 do begin
      case Buffer[I] of
       $0D:LWrite('\r',2);
       $0A:LWrite('\n',2);
       $00..$09,$0B,$0C,$0E..$1F:LWrite('.',2);
       else LWrite(Chr(Buffer[I]),2);
      end;
     end;
     LWrite(')',2);
    end
    else LWrite('<No answer!>',2);
    LWriteln('',2);
    if (Uk<>3) or (Buffer[0]<>$0D) or (Buffer[1]<>$0A) or (Buffer[2]<>$3E) then begin    
     LWriteln('ERROR! Incorrect applet or execution-in-phone failed!',2);
     BootAuth:=-1;
     Exit;
    end;
   end;
   FreeMem(BufferApplet,AppletSize);
   BootAuth:=Res;
  end;
 
 {Функция тестирования работы апплета} 
 function AppletTest(var Port:TBlockSerial):Longint;
  var
   Res:Longint;
   Buffer:TBuffer;
   I,Uk:Longint;
  begin
   Res:=0;
   {Пошлем апплету команду V}
   Sleep(100);
   Port.SendByte($56);
   Sleep(100);
   if Port.LastError<>0 then begin
    LWriteln('ERROR! Cannot send V command!',2);
    AppletTest:=-1;
    Exit;
   end
   else LWrite('V: ',2);
   {Считаем ответ апплета}
   Uk:=ReadCard(Port,Buffer,0);
   if Uk>0 then begin
    for I:=0 to Uk-1 do begin
     LWrite(PrintHexByte(Buffer[I])+' ',2);
    end; 
    LWrite('(',2);
    for I:=0 to Uk-1 do begin
     case Buffer[I] of
      $0D:LWrite('\r',2);
      $0A:LWrite('\n',2);
      $00..$09,$0B,$0C,$0E..$1F:LWrite('.',2);
      else LWrite(Chr(Buffer[I]),2);
     end;
    end;
    LWrite(')',2);
   end
   else LWrite('<No answer!>',2);
   LWriteln('',2);
   {Пошлем апплету команду I}
   Sleep(100);
   Port.SendByte($49);
   Sleep(100);
   if Port.LastError<>0 then begin
    LWriteln('ERROR! Cannot send I command!',2);
    AppletTest:=-1;
    Exit;
   end
   else LWrite('I: ',2);
   {Считаем ответ апплета}
   Uk:=ReadCard(Port,Buffer,0);
   if Uk>0 then begin
    for I:=0 to Uk-1 do begin
     LWrite(PrintHexByte(Buffer[I])+' ',2);
    end; 
    LWrite('(',2);
    for I:=0 to Uk-1 do begin
     case Buffer[I] of
      $0D:LWrite('\r',2);
      $0A:LWrite('\n',2);
      $00..$09,$0B,$0C,$0E..$1F:LWrite('.',2);
      else LWrite(Chr(Buffer[I]),2);
     end;
    end;
    LWrite(')',2);
   end
   else LWrite('<No answer!>',2);
   LWriteln('',2);
   AppletTest:=Res;
  end;
            
begin
end.
