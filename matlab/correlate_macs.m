% - Config -
% Sample Rate (Hz)
Fs = 20e6;
% Time between Samples (sec)
Ts = 1/Fs;
% Bin Time Period (sec)
T = 1e-3;
% Bin Time Period (samples)
delT = T/Ts;
% Number of data sources
N=3;

% - Data Read -
% Read in the acorr_peak timestamps (in absolute samples)
s2l2_acorr = csvread('2462_25_1630_northtocorner_64-12-25-7c-28-1_acorr_peaks.txt');
s1l1_acorr = csvread('2462_25_1630_ssl2_row7seat2_64-12-25-7c-28-1_acorr_peaks.txt');
rfsn1_acorr = csvread('2462_25_1630_rfsn1_64-12-25-7c-28-1_acorr_peaks.txt');

% - Massage invidual vectors to form observation matrix -
s2l2_len = length(s2l2_acorr);
s1l1_len = length(s1l1_acorr);
rfsn1_len = length(rfsn1_acorr);
lv = [s1l1_len;s2l2_len;rfsn1_len];
M = max(lv);
if( s1l1_len < M )
    s1l1_acorr = [s1l1_acorr;zeros(M-s1l1_len,1)];
end

if( s2l2_len < M )
    s2l2_acorr = [s2l2_acorr;zeros(M-s2l2_len,1)];
end

if( s1l1_len < M )
    rfsn1_acorr = [rfsn1_acorr;zeros(M-rfsn1_len,1)];
end

M = max(lv);
acorr = [s1l1_acorr,s2l2_acorr,rfsn1_acorr];

% - Bin Data - 
max_indx = max(acorr(:));
min_indx = 0; %min(s2l2_acorr)
num_bins = ceil((max_indx-min_indx)/delT) + 1
acorr_bins = zeros(num_bins,N);
bin_indx = zeros(N,1);
% For each element in the vector (acorr_peak absolute sample)
 for i=1:M
     % For each sensor
     for j=1:N
        if( i <= lv(j) )
            bin_indx(j) = floor(acorr(i,j)/delT)+1;
            % Increment the count
            acorr_bins(bin_indx(j),j) = acorr_bins(bin_indx(j),j) + 1;
        end
     end
 end
 
 % - Cross Correlations -
 num_xcorrs = nchoosek(3,2);
 xc = zeros(2*num_bins-1,num_xcorrs);
 xc_indx = zeros(num_xcorrs,1);
 
%  for i=1:N-1
%      xc(:,i) = xcorr(acorr_bins(:,i),acorr_bins(:,(i+1))); 
%  end
 xc(:,1) = xcorr(acorr_bins(:,1),acorr_bins(:,2));
 xc(:,2) = xcorr(acorr_bins(:,1),acorr_bins(:,3));
 xc(:,3) = xcorr(acorr_bins(:,2),acorr_bins(:,3));
 
 % Find max peak = time offset
 [xc_val,xc_indx] = max(xc);
 % Since xcorr puts the zeroth lag at num_bins, compensate
 xc_indx = xc_indx - num_bins;
 xc_time = xc_indx.*T;
 
 % - Plots -
 % Number of plots
 num_plots = num_xcorrs+1;
 figure(1);
 subplot(num_plots,1,1);
 stem(acorr_bins);
 xlabel(sprintf('bin number (delT=%d samps=%f secs)',delT,T));
 ylabel('Number of Occurances');
 title('Unaligned');
 legend('s1l1','s2l2','rfsn1');
 
 for i=1:N
    subplot(num_plots,1,i+1);
    plot(xc(:,i));
    corr_label = nchoosek([1 2 3],2);
    plot_title = sprintf('Cross Correlation %d-%d,Max XC @ bin %d=%e s',...
                     corr_label(i,1),corr_label(i,2),xc_indx(i),xc_time(i));
    title(plot_title);
    xlabel('bin number');
    ylabel('correlation');
 end
 
% Create main title 
ha = axes('Position',[0 0 1 1],'Xlim',[0 1],'Ylim',[0 1],'Box','off',...
    'Visible','off','Units','normalized', 'clipping' , 'off');

text(0.5, 1,'\bf 11-1-2014 (Virginia) 2462 MHz @ 16:30 MAC=64:12:25:7c:28:1 (GTWiFi)',...
            'HorizontalAlignment','center','VerticalAlignment', 'top');
