program sample14;
	/* Calculator program	*/
	/*  c n : clear & set n	*/
	/*  + n : add n			*/
	/*  - n : subtract n	*/
	/*  * n : multiply n	*/
	/*  / n : divide n		*/
	/*  o : off				*/
var com :char;
	x, y : integer;
	endflag : boolean;
begin
	writeln('   *** Calculator -- h for help ***');
	x := 0;
	endflag := false;
	while not endflag do begin
		writeln(' Please input command :');
		readln(com, y);
		if (com = 'c')  or (com = 'C') then begin
			x := y;
		end
		else if com = '+' then begin
			x := x + y;
		end
		else if com = '-' then begin
			x := x - y;
		end
		else if com = '*' then begin
			x := x * y;
		end
		else if com = '/' then begin
			x := x / y;
		end
		else if (com = 'o') or (com = 'O') then begin
			endflag := true
		end
		else begin
			writeln;
			writeln('Calculator Usage:');
			writeln('  c number : clear & set it');
			writeln('  + number : add it');
			writeln('  - number : subtract it');
			writeln('  * number : multiply it');
			writeln('  / number : divide it');
			writeln('  o        : off(terminate execution)');
			writeln
		end;
		if endflag then
			writeln('Final Result =', x)
		else
			writeln('Temporary Result =', x)
	end
end.

