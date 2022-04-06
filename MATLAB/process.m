clearvars
%% Constants
buffersize = 1024;
emgdatainsize = 10;
emgdataoutsize = 3;
% udp ports
esp32 = udpport('datagram','OutputDatagramSize',buffersize,'LocalPort',12345);
% data storage
sensormatrix = zeros(2^20, 3);
rows = ones(1, 1);

%% Take incoming data
while rows < height(sensormatrix)
    if (esp32.NumDatagramsAvailable ~= 0)
        % read data from esp32
        data = uint8(read(esp32,1,'uint8').Data);

        i = 1;
        while i <= length(data)
            header = data(i:i+1);
            if header ~= 0xAA
                break
            end
            id = double(data(i+2));
            if id == 1
                counter = typecast(data(i+3:i+6),'uint32');
                mag = typecast(data(i+7:i+10),'uint16');
                
                i = i + emgdatainsize;

                l = emgdataoutsize * (id - 1) + 1;
                r = l + emgdataoutsize - 1;
                sensormatrix(rows(id), l:r) = [cast(counter,'double') cast(quat, 'double')];
                rows(id) = rows(id) + 1;
            end
        end
    end
end