
% shamelessly stolen and modified from the web
function parameters = readElastixParameters(parameterFile)

fid = fopen(parameterFile);

tline = fgetl(fid);
while isempty(tline)
    tline = fgetl(fid);
    while regexp(tline,'^//')
        tline = fgetl(fid);
    end
end

while ischar(tline)
    %Skip comment lines and following empty lines. Extra break
    %statements are to handle comments at the end of the file
    while regexp(tline,'^//')
        tline = fgetl(fid);
        while isempty(tline)
            tline = fgetl(fid);
        end
        if ~ischar(tline), break, end
    end
    if ~ischar(tline), break, end
    
    
    
    % - - - - - - - - - - - - - - - - - - - - - - - - -
    %Get the data
    %Extract parameter name
    tok=regexp(tline,'\((\w+)','tokens');
    param=tok{1}{1};
    
    if strfind(tline,'"') %Then it's a string value
        tok=regexp(tline,'"(.*)"','tokens');
        value=tok{1}{1};
    else %It's a numeric value
        tok=regexp(tline,'\w+ ([\d\. -]+)\)','tokens');
        value=str2num(tok{1}{1}); %#ok
    end
    % - - - - - - - - - - - - - - - - - - - - - - - - -
    
    
    
    parameters.(param)=value; %Add to structure
    tline=fgetl(fid); %Read next line
    
    %Skip empty lines
    while isempty(tline), tline=fgetl(fid); end
end

fclose(fid);
end

