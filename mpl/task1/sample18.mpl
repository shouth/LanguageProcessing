program primefactor;	{ program18 }
var factor : array[200] of integer;
	i, n, can : integer;
begin
	i := 0;
	while i < 200 do begin
		factor[i] := 0;
		i := i + 1
	end;
	writeln('Input positive integer');
	readln(n);
	can := 2;
	while (can <= n div 2) and (can < 200) do begin
		if (n - (n div can) * can) <> 0 then can := can + 1
		else begin
			factor[can] := factor[can] + 1;
			n := n div can
		end
	end;
	i := 0;
	if n < 200 then begin
		factor[n] := factor[n]+1;
		n := 1
	end;
	while i < 200 do begin
		if factor[i] <> 0 then begin
			writeln('    ', i, ' ** ', factor[i]);
		end;
		i := i + 1
	end;
	if n > 1 then writeln('    ', n, ' ** ', 1)
end.
