program sample13; { square root }
var x, high, low, mid, can : integer;
begin
	writeln('Input x for calculating root x');
	readln(x);
	if x < 0 then begin
		writeln('can not calculate a root of negative number')
	end
	else begin
		low := 0;
		high := 181;
		while (high - low) >= 2 do begin
			mid := (high + low) div 2;
			can := mid * mid;
			if x < can then high := mid
			else if can < x then low := mid
			else begin
				high := mid;
				low := mid
			end
		end;
		if high = low then writeln('root ', x, ' = ', low)
		else if (high * high - x) > (x - low * low) then
			writeln('root ', x, ' = ', low)
		else writeln('root ', x, ' = ', high)
	end
end.
				
