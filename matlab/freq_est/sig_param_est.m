function [fhatArr, varfhatArrCRL, phihatArr, varphihatArrCRL, ahatArr, varahatArrCRL,DFT_windowed_x] = sig_param_est(x, variance_of_whiteNoise, samplingRate,windowSize)
blkN = 1;
[N,cols] = size(x);  %complex signal length
fhatArr = [];
varfhatArrCRL =  [];
phihatArr = [];
varphihatArrCRL = [];
ahatArr = [];
varahatArrCRL = [];
loopCount = 0;
% windowFunc = hann(windowSize, 'periodic');
windowFunc = ones(windowSize,1);
N = (length(x) - mod(length(x),windowSize));  %file length rounded of to highest length divisible by windowSize
    for i = 1:N/windowSize
        NFFT=4*windowSize;
        windowed_x  = windowFunc .* x(blkN:(blkN+windowSize-1));
        blkN = blkN + windowSize;
        DFT_windowed_x1 = fft(windowed_x, NFFT);    % Compute DFT with 2x window
        DFT_windowed_x = fftshift(DFT_windowed_x1);
        [max_peak,max_arg] = max(abs(DFT_windowed_x));            % Freq estimate
        n_norm = (-NFFT/2:NFFT/2-1)./NFFT;
%       fhat = n_norm(max_arg); % scaled freq
        fhat = (n_norm(max_arg))*samplingRate; % freq in Hz
        % %phihat = unwrap(angle(DFT_windowed_x));   % Phase
    
        % The following equations are from Paul's paper on Sinusoidal
        % Parameter Estimation which referenced: "D. Rife and R.R. Boorstyn Single tone 
        % parameter estimation from discrete-time observations        
        ahat = sum(windowed_x.*(exp(-i*2*pi*(fhat/samplingRate)*(1:windowSize)')))/windowSize;
        phihat = atan2(imag(ahat*windowSize),real(ahat*windowSize));
        varfhatCRL = 6*(variance_of_whiteNoise)/((2*pi.^2)*windowSize*(windowSize.^2 - 1)*max(abs(windowed_x))^2);
        varphihatCRL = (variance_of_whiteNoise)*(2*windowSize -1)/((2*pi.^2)*windowSize*(windowSize + 1)*max(abs(windowed_x))^2);
        varahatCRL = (variance_of_whiteNoise)/(2*windowSize);
        fhatArr = [fhatArr, fhat];
        varfhatArrCRL =  [varfhatArrCRL, varfhatCRL];
        phihatArr = [phihatArr, phihat];
        varphihatArrCRL = [varphihatArrCRL, varphihatCRL];
        ahatArr = [ahatArr, ahat];
        varahatArrCRL = [varahatArrCRL, varahatCRL];
        loopCount = loopCount+1;
    end
end
