clear all 
close all
clc
% % Single file test case
% complexSignal = read_cshort_binary ('/home/user1/Documents/vipSpring15/freq_estV2/freq_estim_03302015/freq_estim_03302015_d1/2437_4_400KHzfreqEstRFSNd1_03302015.sc16');
% % [fhatArr, varfhatArr, phihatArr, varphihatArr, ahatArr, varahatArr, DFT_windowed_x] = sig_param_est(complexSignal, variance_of_whiteNoise, samplingRate, windowSize);
% [fhatArr, varfhatArr, phihatArr, varphihatArr, ahatArr, varahatArr, DFT_windowed_x] = sig_param_est(complexSignal, 39, 4e6, 2048);

noiseFile=6;  % noise file number in array of data files
samplingRate=4e6;
% windowSizeArr=[1024:1024:5120];
windowSizeArr=[2048];
NumOfNodes = 2;
nd=1; nf=1;

% The following lines try to generate commplete file paths
% Data directories. Please change to directory containing data files
directoryPaths = {
    '/home/user1/Documents/vipSpring15/freq_estV2/freq_estim_03302015/freq_estim_03302015_d1/';
    '/home/user1/Documents/vipSpring15/freq_estV2/freq_estim_03302015/freq_estim_03302015_d2/';
    ''
    };
% directoryPath3 = '/home/user1/Documents/vipSpring15/freq_est/freq_estim_03302015/freq_estim_03302015_d3/';

% Data files. Please change names in the following array to actual file
% names in the directory specified earlier
fileNameArray1 = {
    '2437_4_100KHzfreqEstRFSNd1_03302015.sc16';
    '2437_4_200KHzfreqEstRFSNd1_03302015.sc16';
    '2437_4_300KHzfreqEstRFSNd1_03302015.sc16';
    '2437_4_400KHzfreqEstRFSNd1_03302015.sc16';
    '2437_4_500KHzfreqEstRFSNd1_03302015.sc16';
    '2437_4_NOISEfreqEstRFSNd1_03302015.sc16'
    };
fileNameArray2 = {
    '2437_4_100KHzfreqEstRFSNd2_03302015.sc16';
    '2437_4_200KHzfreqEstRFSNd2_03302015.sc16';
    '2437_4_300KHzfreqEstRFSNd2_03302015.sc16';
    '2437_4_400KHzfreqEstRFSNd2_03302015.sc16';
    '2437_4_500KHzfreqEstRFSNd2_03302015.sc16';
    '2437_4_NOISEfreqEstRFSNd2_03302015.sc16'
    };
% Leave as is or entre names of files colleced by the third node
fileNameArray3 = {''; ''; ''; ''; ''; ''};

% This forms a cell nfxnd matrix with file names(nf) from all nodes(nd)
filePathsArray = cat(2, fileNameArray1, fileNameArray2, fileNameArray3);
while (nd<=NumOfNodes)   % this loop generates the complete file paths for the file name matrix
    while (nf<=length(filePathsArray))
        filePathsArray(nf,nd) = strcat(directoryPaths(nd),filePathsArray(nf,nd));
        nf = nf+1;
    end
nd = nd+1;
nf=1;
end

% This computes the covariance matrix for the noise file collected at each
% node
for windowIndx=1:length(windowSizeArr)
    windowSize = windowSizeArr(windowIndx);
    
    nd=1; %node number
    nf=1; %file number
    variance_of_whiteNoise=0;
    if(windowIndx<=1 && noiseFile>=1 )
        complexNoise = read_cshort_binary(filePathsArray{noiseFile,nd});
        complexNoise_reshaped = (reshape(complexNoise(1: length(complexNoise) - mod(length(complexNoise),windowSize)),[windowSize,floor(length(complexNoise)/windowSize)]))';
        complexNoise_cov = cov(abs(complexNoise_reshaped));
        variance_of_whiteNoise = mean(diag(complexNoise_cov));
    end
    
% % complexSignal = read_cshort_binary(filePathsArray{1,1});
% % [fhatAvg, varfhatAvg, phihatAvg, varphihatAvg, ahatAvg, varahatAvg] = sig_param_est(complexSignal, variance_of_whiteNoise, samplingRate, windowSize)

    
    while (nd<=NumOfNodes)
        while (nf<=length(filePathsArray))
            complexSignal = read_cshort_binary(filePathsArray{nf,nd});
            [fhatArr, varfhatArrCRL, phihatArr, varphihatArrCRL, ahatArr, varahatArrCRL, DFT_windowed_x] = sig_param_est(complexSignal, variance_of_whiteNoise, samplingRate, windowSize);
            s1= strcat(' for node_',num2str(nd),' file_',num2str(nf));
            if(nd<=1)
                figure(nf)
            else
                figure(((nd-1)*length(filePathsArray)) + nf)
            end
            subplot(4,2,1)
            plot(fhatArr)
            title(strcat('fhat',s1))
            xlabel('Window number') % x-axis label
            ylabel('fhat (Hz)') % y-axis label
            subplot(4,2,2)
            plot(varfhatArrCRL)
            title(strcat('varfhatArrCRL',s1))
            xlabel('Window number') % x-axis label
            ylabel('varfhat (Hz)') % y-axis label
            subplot(4,2,3)
            plot(phihatArr)
            title(strcat('phihatArr',s1))
            xlabel('Window number') % x-axis label
            ylabel('phihat (rad)') % y-axis label
            subplot(4,2,4)
            plot(varphihatArrCRL)
            title(strcat('varphihatArrCRL',s1))
            xlabel('Window number') % x-axis label
            ylabel('varphihat (rad)') % y-axis label
            subplot(4,2,5)
            plot(abs(ahatArr))
            title(strcat('ahatArr',s1))
            xlabel('Window number') % x-axis label
            ylabel('ahat (magnitude)') % y-axis label
            
% %             subplot(4,2,6)
% %             plot(varahatArrCRL)
% %             title(strcat('varahatArr',s1))
% %             xlabel('Window number') % x-axis label
% %             ylabel('varahat (Hz)') % y-axis label

            subplot(4,2,6)
            plot(abs(DFT_windowed_x))
            title(strcat('DFT_windowed_x for last window',s1))
            xlabel('Padded FFT window NFFT = 4*FFTwindow') % x-axis label
            subplot(4,2,7)
            histogram(fhatArr)
            title(strcat('Histogram fhatArr',s1))
            xlabel('Freq distribution fhat') % x-axis label
            ylabel('') % y-axis label
            subplot(4,2,8)
            imagesc(complexNoise_cov)
            title(strcat('Noise cov',s1))
            
            fhatAvg = sum(fhatArr)/length(fhatArr);
            varfhat =  var(fhatArr);
            phihatAvg = sum(phihatArr)/length(phihatArr);
            varphihat = var(phihatArr);
            ahatAvg = sum(ahatArr)/length(ahatArr);
            varahatAvg = sum(ahatArr)/length(ahatArr);
            fprintf('For file %d, on node %d \n',nf ,nd);
            fprintf('fhatAvg : %d ;varfhat : %d ;phihatAvg : %d ;varphihat : %d ;ahatAvg : %d ;varahatAvg : %d ; \n'...
                , fhatAvg,varfhat,phihatAvg,varphihat,ahatAvg,varahatAvg);
            nf = nf+1;
        end
        nd = nd+1;
        nf=1;
    end
end
