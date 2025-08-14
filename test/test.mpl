program sample11;
var n, sum, data : integer;
begin
	writeln('input the number of data');
	readln(n);
	sum := 0;
	while n > 0 do begin
		readln(data);
		sum := sum + data;
		n := n - 1
	end;
	writeln('Sum of data = ', sum)
end.
