program sample11pp;
procedure kazuyomikomi(n : integer);
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
procedure goukei(n, s : integer);
	var data : integer;
begin
	s := 0;
	while n > 0 do begin
		readln(data);
		s := s + data;
		n := n - 1
	end
end;
var n : integer;
begin
	call kazuyomikomi(n);
	call goukei(n * 2, sum);
	call wakakidasi
end.
