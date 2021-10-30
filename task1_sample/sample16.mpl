program sample16;	/* prime numbers */
var furui : array[20000] of boolean;
    i, j : integer;
begin
  i := 2;
  while i < 20000 do begin
    furui[i] := true;
    i := i + 1
  end;
  furui[0] := false;
  furui[1] := false;
  i := 2;
  while i < 20000 do begin
    if furui[i] then begin
      writeln(i, ' is a prime number');
      j := i;
      if i < 16384 then
        while j < 20000 do begin
          furui[j] := false;
          j := j + i
        end
    end;
    i := i + 1
  end
end.
