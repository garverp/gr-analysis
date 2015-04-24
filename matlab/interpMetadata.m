%fileName is a string corresponding to the name of a .csv file, with ',' in
%between each value
%sampleNum is the item number(s) of the sample(s) to interpolate 
%the second input can be a matrix in any order
%output is the corresponding samples' tsec and tfrac respectively
function [tsec,tfrac]  = interpMetadata(fileName, sampleNum)
    %assumes that the input file is separated by commas
    file = dlmread(fileName, ',',1,0); %can change the delimiter here
    fh = fopen(fileName,'r');
    headers = fgetl(fh); %assumes that the field names will be in the first row
    headers = strsplit(headers, ',');
    file = num2cell(file);
    data = cell2struct(file,headers,2); %Field names can be in any order
    %Structure array in order of Item_num
    %Must have 4 exact field names: Item_num, bytes, rx_tsecs, rx_tfrac
    %All other fields are also added, but currently unused
    error = [];
    [~, cols] = size(file);
    
    for a = 1:length(sampleNum)
        sample = sampleNum(a);
        count = 0;
        %All item numbers that are not within the range of data collected
        %output -1
        
        %Invalid samples do not interfere with the interpolation
        %for any other samples
        if (sample < data(1).Item_num || sampleNum(a) > data(end).Item_num)
                tsec(a) = -1;
                tfrac(a) = -1;
        else
            for b = 1:cols - 2
                if sample > data(b).Item_num && sample < data(b+1).Item_num
                    if (data(b+1).Item_num - data(b).Item_num > data(b).bytes && sample - data(b).Item_num > data(b).bytes)
                        error = [error sample];
                    end
                tsec(a) = interp1([data(b).Item_num data(b+1).Item_num], [data(b).rx_tsecs data(b+1).rx_tsecs],sample);
                tfrac(a) = interp1([data(b).Item_num data(b+1).Item_num], [data(b).rx_tfrac data(b+1).rx_tfrac],sample);
                end
            end
        end
    end
    %Discontinuity is implied when the number of bytes between two items
    %is less than the difference in item number
    
    %The dropped samples are assumed to be at the end of the interval
    %For example: if there are 1000 bytes between two samples that are
    %1200 item numbers apart, only samples 1001-1200 will cause a warning
    if length(error) == 1
        warning = sprintf('Item number %d implies discontinuity',error)
    elseif length(error) > 1
        error = mat2str(error);
        error(error == '[' | error == ']') = '';
        error(error == ' ') = ',';
        warning = sprintf('Item numbers %s imply discontinuity', error)
    end
end
