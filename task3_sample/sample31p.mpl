program sample31p;
var a : integer;
procedure p(a : char);
begin
	writeln('proc of p');
	a := 'a'
end;
var b : char;
procedure q(b:integer);
  var a : boolean;
      q : integer;
begin
	writeln('proc of q');
	readln(q);
	a := b = q;
	if a then writeln('true') else writeln('false')
end;
var c : integer;
begin
	a := 1;		b := 'b';
	call p(b);
	call q(a);call q(2*a+1)
end.
