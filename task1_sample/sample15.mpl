program numberlist;  { sample15 }
var e, n: integer;
begin
	writeln('Number list');
	writeln('    n  ', '   2n  ', ' n**2  ', ' n**3  ', ' 2**n  ');
	n := 0;
	e := 1;
	while n < 15 do begin
		writeln(n:6, ' ', 2*n:6, ' ', n*n:6, ' ', n*n*n:6, ' ', e:6);
		e := e * 2;
		n := n + 1;
	end
end.

