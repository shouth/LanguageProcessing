program IfstTC;	{sample25t}
var
 ch:char;int:integer;boolx,booly:boolean;
begin                             
boolx:=true;booly:=false;ch:='a';int:=66;
write(integer(ch));write(integer(int));write(integer(boolx),integer(booly));
writeln;
writeln(char(ch),char(int));writeln(boolean(ch),integer(int),integer(boolx),
integer(booly));end.
