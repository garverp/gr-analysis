# Files to plot
rfsn1_macs='rfsn1_virginia_macs.txt';
s1l1_macs='s1l1_virginia_macs.txt';
s2l2_macs='s2l2_virginia_macs.txt';
mac_column_lookup={'RFSN1','s1l1','s2l2'};
mac_table={};
num_macs=0;


% Build RFSN1 MAC table
fprintf('Building RFSN1 MAC table...\n');
fileid = fopen(rfsn1_macs,'r');
cline = fgetl(fileid);
while ischar(cline)
   % Split CSV tokens
   C = strsplit(cline,',');
   % Remove colons in the MAC addresses
   C = strrep(C,':','');
   for i=1:length(C)
      intable = strcmp(C{i},mac_table);
      if isempty(find(intable))
         %fprintf('Adding MAC to table: %s\n',C{i});
         num_macs = num_macs + 1;
         mac_table{num_macs,1} = C{i};
         mac_table{num_macs,2} = 1;
         % Need to make the columns for the other sensors
         mac_table{num_macs,3} = 0;
         mac_table{num_macs,4} = 0;
      else
         indx = find(intable);
         %fprintf('MAC %s already exists in location %d\n',C{i},indx);
         mac_table{indx,2} = mac_table{indx,2} + 1;
      end 
   end
   cline = fgetl(fileid);
end
fclose(fileid);

% Build s1l1 MAC table
fprintf('Building S1L1 MAC table...\n');
fileid = fopen(s1l1_macs,'r');
cline = fgetl(fileid);
while ischar(cline)
   C = strsplit(cline,',');
   C = strrep(C,':','');
   for i=1:length(C)
      intable = strcmp(C{i},mac_table);
      if isempty(find(intable))
         %fprintf('Adding MAC to table: %s\n',C{i});
         num_macs = num_macs + 1;
         mac_table{num_macs,1} = C{i};
         mac_table{num_macs,3} = 1;
         % Other sensors have seen zero packets
         mac_table{num_macs,2} = 0;
         mac_table{num_macs,4} = 0;
      else
         indx = find(intable);
         %fprintf('MAC %s already exists in location %d\n',C{i},indx);
         mac_table{indx,3} = mac_table{indx,3} + 1;
      end 
   end
   cline = fgetl(fileid);
end
fclose(fileid);

% Build s2l2 MAC table
fprintf('Building S2L2 MAC table...\n');
fileid = fopen(s2l2_macs,'r');
cline = fgetl(fileid);
while ischar(cline)
   C = strsplit(cline,',');
   C = strrep(C,':','');
   for i=1:length(C)
      intable = strcmp(C{i},mac_table);
      if isempty(find(intable))
         %fprintf('Adding MAC to table: %s\n',C{i});
         num_macs = num_macs + 1;
         mac_table{num_macs,1} = C{i};
         mac_table{num_macs,4} = 1;
         % Other sensors have seen zero packets
         mac_table{num_macs,2} = 0;
         mac_table{num_macs,3} = 0;
      else
         indx = find(intable);
         %fprintf('MAC %s already exists in location %d\n',C{i},indx);
         mac_table{indx,4} = mac_table{indx,4} + 1;
      end 
   end
   cline = fgetl(fileid);
end
fclose(fileid);
mac_raster = cell2mat(mac_table(:,2:4));

