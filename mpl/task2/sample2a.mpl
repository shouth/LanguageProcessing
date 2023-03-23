program change;  /* sample2a.mpl */ { compute change program. What is change? }
             { no 5, 50, 500, & 5000 yen used }
  var n, count : integer;
begin
  writeln('please input change');  readln(n);    count := 0;
while n>0 do begin
while n>=10 do begin
while n>=  100 do begin
while n>=1000 do begin  
while n  >=10000 do begin
               count := count +1; n := n-10000;
             end;
             if count >0 then writeln('10000 yen : ',  count);
             count := 0;
             if n < 1000 then break;
             count := count +1; n := n-1000;
           end;
             if count >0 then writeln(' 1000 yen : ',  count);
             count := 0;
             if n < 100 then break;
              count := count +1; n := n-100;
         end;
         if count >0 then     writeln('  100 yen : ',  count);
             count := 0;
             if n < 10 then break;
              count := count +1; n := n-10;
      end;




         if count >0 then     writeln('   10 yen : ',  count);
             count := 0;
             if n < 1 then break;
              count := count +1; n := n-1;
 end;
 if count >            0 then writeln('    1 yen : ',  count);
/*  if n !=0 then break;  */
end.