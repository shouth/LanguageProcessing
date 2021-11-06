program sample29;
	/* Calculator program	*/
	/*  c n : clear & set n	*/
	/*  + n : add n			*/
	/*  - n : subtract n	*/
	/*  * n : multiply n	*/
	/*  / n : divide by n		*/
	/*  o : off				*/
var unused1: integer;
    UnusedArrayForTest: array[200] of char;
procedure gcmlcm(m, n, gc, lc : integer);
	/* gc := GCM(m,n), lc := LCM(m,n) */
	/* m and n are not changed        */
var a,b,r : integer;
begin
a := m;
b := n;
while b <> 0 do begin
r := a - (a div b) * b;
a := b;
b := r
end;
gc := a;
lc := (m div gc) * n
end;
procedure abs(a, b : integer);
begin
if a < 0 then b := -a  else b := a
end;
procedure gcm(a, b, gc : integer);
var lc, aa, bb : integer;
begin
if (a = 0) or (b = 0) then gc := 1
else begin
call abs(a,aa);  call abs(b,bb);
call gcmlcm(aa, bb, gc, lc)
end
end;
procedure lcm(a, b, lc : integer);
var gc, aa, bb : integer;
begin
if (a = 0) or (b = 0) then lc := 1
else begin
call abs(a,aa);  call abs(b,bb);
call gcmlcm(aa, bb, gc, lc)
end
end;
var unusedchar : char;
procedure reduce(a1, a2 : integer);
  var gc:integer;
begin
if a1 = 0 then begin a2 := 1; return end;
if a2 = 0 then begin a1 := 1; return end;
if a2 < 0 then begin a1 := -a1;      a2 := - a2 end;
 call gcm(a1, a2, gc);
a1 := a1 div gc;
a2 := a2 div gc
end;

procedure sum(x1, x2, y1, y2 : integer);
var lc, y11 : integer;
begin
call lcm(x2, y2, lc);
x1 := x1 * (lc div x2);
y11 := y1 * (lc div y2);
x1 := x1 + y11;
x2 := lc;
call reduce(x1, x2)
end;
procedure sub(x1, x2, y1, y2 : integer);
var lc, y11 : integer;
begin
call sum(x1, x2, -y1, y2)
end;
procedure mult(x1, x2, y1, y2 : integer);
var gc,y22,y11 : integer;
begin
call gcm(x1, y2, gc);
x1 := x1 div gc;
y22 := y2 div gc;
call gcm(x2, y1, gc);
x2 := x2 div gc;
y11 := y1 div gc;
x1 := x1 * y11;
x2 := x2 * y22;
call reduce(x1, x2)
end;
procedure divide(x1, x2, y1, y2 : integer);
begin
call mult(x1, x2, y2, y1)
end;
var unusedarray : array[100] of char;
procedure printfinal(a, b : integer);
begin
if a = 0 then writeln('Final Result =', a)
else if b = 1 then writeln('Final Result =', a)
else writeln('Final Result =', a, '/', b)
end;
procedure printtemp(a, b : integer);
begin
if a = 0 then writeln('Temporary Result =', a)
else if b = 1 then writeln('Temporary Result =', a)
else writeln('Temporary Result =', a, '/', b)
end;

var	x1, x2, y1,y2 : integer;
var	com :char;
endflag : boolean;
begin
writeln('   *** Calculator -- h for help ***');
x1 := 0;  x2 :=1;
endflag := false;
while not endflag do begin
writeln(' Please input command :');
readln(com, y1);  y2 := 1;
if (com = 'c')  or (com = 'C') then begin x1 := y1; x2 := y2 end
else if com = '+' then call sum(x1, x2, y1, y2)
else if com = '-' then call sub(x1, x2, y1, y2)
else if com = '*' then call mult(x1, x2, y1, y2)
else if com = '/' then call divide(x1, x2, y1, y2)
else if (com = 'o') or (com = 'O') then endflag := true
else begin
writeln;
writeln('Calculator Usage:');
writeln('  c number : clear & set it');
writeln('  + number : add it');
writeln('  - number : subtract it');
writeln('  * number : multiply it');
writeln('  / number : divide by it');
writeln('  o        : off(terminate execution)');
writeln
end;
if endflag then call printfinal(x1, x2)
else call printtemp(x1, x2)
end
end.
