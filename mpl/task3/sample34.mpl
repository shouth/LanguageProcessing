program sample34;
var num : integer;  ch : char;

begin
	writeln('input the number of data and data character (readln)');
	readln(num);  readln(ch);
	writeln;
	while num > 0 do begin
		write(ch);
		num := num - 1
	end;
	writeln;
	writeln('input the number of data and data character (read)');
	read(num);  read(ch);
	writeln;
	while num > 0 do begin
		write(ch);
		num := num - 1
	end;
	writeln
end.
