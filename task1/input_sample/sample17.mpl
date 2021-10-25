program GCMLCM;		/* sample17 */
var m, n, a, b, r : integer;
	gcm, lcm : integer;
begin
	writeln('Input two integers');
	readln(m, n);
	a := m;
	b := n;
	while b <> 0 do begin
		r := a - (a div b) * b;
		a := b;
		b := r
	end;
	gcm := a;
	lcm := (m div gcm) * n;
	writeln('GCM = ', gcm, '   LCM = ', lcm)
end.

