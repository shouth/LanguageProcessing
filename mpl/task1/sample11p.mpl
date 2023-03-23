program sample11p;
var n : integer;
procedure kazuyomikomi;
begin
	writeln('input the number of data');
	readln(n)
end;
var sum : integer;
procedure wakakidasi;
begin
	writeln('Sum of data = ', sum)
end;
var data : integer;
procedure goukei;
begin
	sum := 0;
	while n > 0 do begin
		readln(data);
		sum := sum + data;
		n := n - 1
	end
end;
begin
	call kazuyomikomi;
	call goukei;
	call wakakidasi
end.
