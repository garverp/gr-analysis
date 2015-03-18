%fileName is a string corresponding to the name of a .csv file, with ; in
%between each value
%sampleNum is the index of the sample(s) to interpolate (STARTS AT 1, NOT 0)
%the second input can also be a matrix
%output is the corresponding sample's tsec and tfrac respectively
%assumes 
function [tsec,tfrac]  = interpMetadata(fileName, sampleNum)
    %columns are tsecs, tfrac, number of elements, rx_freq, and
    %rx_rate in that order
    %assumes that the input file is separated by semicolons
    data = dlmread(fileName, ';'); %can change the delimiter here
    numSamples = data(1,3);
    data(:,6) = 0;
    data(1,6) = 1;
    [rows, ~] = size(data);
    %adds a separate column for sample number, assuming no samples are
    %missing
    for z = 2:rows
        rate = data(z,5);
        data(z,6) = data(z-1,6) + rate;
    end
    if(min(sampleNum) < 1 | max(sampleNum) > numSamples)
        error = 'Input has one or more invalid sample numbers'
    %should happen every time if used as intended
    else
        place = 1;       
        for sampleSpot = [1:length(sampleNum)]
            place = 1;
            if (sampleNum(sampleSpot) < 
            elseif (any(find(data(:,6) == sampleNum(sampleSpot))))
                place = find(data(:,6) == sampleNum(sampleSpot));
                tsec(sampleSpot) = data(place,1);
                tfrac(sampleSpot) = data(place,2);
            else
                for i = [1:rows - 1]
                    if (data(i,6) < sampleNum(sampleSpot) & data(i+1,6) > sampleNum(sampleSpot))
                        place = i;
                    end
                end
                tsec(sampleSpot) = interp1(data(place:place+1,6),data(place:place+1,1),sampleNum(sampleSpot));
                tfrac(sampleSpot) = interp1(data(place:place+1,6),data(place:place+1,2),sampleNum(sampleSpot));
            end

        end

    end
end
