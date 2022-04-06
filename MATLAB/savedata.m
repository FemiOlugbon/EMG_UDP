%% File name
filename = "maintrial-04-05-22";

%% Data Preparation
sendervars = strings(1,emgdataoutsize);
for i = 1:1
    senderstr = num2str(i);
    sendervars(1) = strcat('counter_',senderstr);
    sendervars(2) = strcat('emg0_',senderstr);
    sendervars(3) = strcat('emg1_',senderstr);

    l = emgdataoutsize * (i - 1) + 1;
    r = l + emgdataoutsize - 1;
    tempmatrix = sensormatrix(:,l:r);
    % remove rows with all zeros
    tempmatrix(all(~tempmatrix,2), :) = [];
    sensortable = array2table(tempmatrix,'VariableNames',sendervars);
    writetable(sensortable,strcat(strcat('data','/'),strcat(filename,'_'),strcat(senderstr,'.csv')));
end