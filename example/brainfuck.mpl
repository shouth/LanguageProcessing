program brainfuck;
var
    size: integer;
    cells: array[30000] of char;
    ptr: integer;
    src: array[30000] of char;
    cursol: integer;

procedure load;
var
    i, jump: integer;
begin
    read(size);
    i := 0;
    jump := 0;
    while i < size do
    begin
        read(src[i]);
        if src[i] = '[' then
            jump := jump + 1
        else if (src[i] = ']') and (jump > 0) then
            jump := jump - 1
        else
        begin
            writeln('mismatching square parenthesis');
            return
        end;
        i := i + 1
    end;
    if jump > 0 then
    begin
        writeln('unclosed square parenthesis');
        return
    end;
end;

begin
    call load;
    cursol := 0;
    ptr := 0;
    while cursol < size do
    begin
        if src[cursol] = '>' then
            ptr := ptr + 1
        else if src[cursol] = '<' then
            ptr := ptr - 1
        else if src[cursol] = '+' then
            cells[ptr] := char(integer(cells[ptr]) + 1)
        else if src[cursol] = '-' then
            cells[ptr] := char(integer(cells[ptr]) - 1)
        else if src[cursol] = '.' then
            write(cells[ptr])
        else if src[cursol] = ',' then
            read(cells[ptr])
        else if (src[cursol] = '[') and (integer(cells[ptr]) = 0) then
            while src[cursol] <> ']' do
                cursol := cursol + 1
        else if (src[cursol] = ']') and (integer(cells[ptr]) <> 0) then
            while src[cursol] <> '[' do
                cursol := cursol - 1;
        cursol := cursol + 1;
    end
end.
