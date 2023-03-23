program nwhilest;  {sample27}
var i, j, k    : integer;
begin
i := 1;
while i <10 do begin
j := 1;
while j < 10 do begin
k := 1;
while k < 10 do begin 
if (k div 2)*2=k then begin k := k+1 end
else begin k:=k+1end
end;
j := j+1;
end;
i:=i+1
end;
writeln                    ('All End')end.
