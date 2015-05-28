% Files to plot
base_dir='/home/data/working/spring15game/overlap_macs/'
node1_macs=strcat(base_dir,'rfsn1_epoch2.macs');
node2_macs=strcat(base_dir,'rfsn-mobile1_epoch2.macs');
node3_macs=strcat(base_dir,'rfsn-mobile2_epoch2.macs');
% Config
% Verbosity
% 0 = Minimal Output
% 1 = Output when new MAC is found
% 2 = Output every MAC
verbose=0;
% Variables
mac_table={};
num_macs=0;



% Build Node1 MAC table
fprintf('Building Node1 MAC table...\n');
fileid = fopen(node1_macs,'r');
cline = fgetl(fileid);
while ischar(cline)
   % Split CSV tokens
   C = strsplit(cline,',');
   % Remove colons in the MAC addresses
   C = strrep(C,':','');
   for i=1:length(C)
      intable = strcmp(C{i},mac_table);
      if isempty(find(intable))
         if verbose == 1
            fprintf('Adding MAC to table: %s\n',C{i});
         end
         num_macs = num_macs + 1;
         mac_table{num_macs,1} = C{i};
         mac_table{num_macs,2} = 1;
         % Need to make the columns for the other sensors
         mac_table{num_macs,3} = 0;
         mac_table{num_macs,4} = 0;
      else
         indx = find(intable);
         if verbose == 2
            fprintf('MAC %s already exists in location %d\n',C{i},indx);
         end
         mac_table{indx,2} = mac_table{indx,2} + 1;
      end 
   end
   cline = fgetl(fileid);
end
fclose(fileid);

% Build Node2 MAC table
fprintf('Building Node2 MAC table...\n');
fileid = fopen(node2_macs,'r');
cline = fgetl(fileid);
while ischar(cline)
   C = strsplit(cline,',');
   C = strrep(C,':','');
   for i=1:length(C)
      intable = strcmp(C{i},mac_table);
      if isempty(find(intable))
         if verbose == 1
            fprintf('Adding MAC to table: %s\n',C{i});
         end
         num_macs = num_macs + 1;
         mac_table{num_macs,1} = C{i};
         mac_table{num_macs,3} = 1;
         % Other sensors have seen zero packets
         mac_table{num_macs,2} = 0;
         mac_table{num_macs,4} = 0;
      else
         indx = find(intable);
         if verbose == 2
            fprintf('MAC %s already exists in location %d\n',C{i},indx);
         end
         mac_table{indx,3} = mac_table{indx,3} + 1;
      end 
   end
   cline = fgetl(fileid);
end
fclose(fileid);

% Build Node3 MAC table
fprintf('Building Node3 MAC table...\n');
fileid = fopen(node3_macs,'r');
cline = fgetl(fileid);
while ischar(cline)
   C = strsplit(cline,',');
   C = strrep(C,':','');
   for i=1:length(C)
      intable = strcmp(C{i},mac_table);
      if isempty(find(intable))
         if verbose == 1
            fprintf('Adding MAC to table: %s\n',C{i});
         end
         num_macs = num_macs + 1;
         mac_table{num_macs,1} = C{i};
         mac_table{num_macs,4} = 1;
         % Other sensors have seen zero packets
         mac_table{num_macs,2} = 0;
         mac_table{num_macs,3} = 0;
      else
         indx = find(intable);
         if verbose == 2
            fprintf('MAC %s already exists in location %d\n',C{i},indx);
         end
         mac_table{indx,4} = mac_table{indx,4} + 1;
      end 
   end
   cline = fgetl(fileid);
end
fclose(fileid);
% Locate overlapping MACS 
rows = find( arrayfun(@(RIDX) mac_table{RIDX,2} ~= 0 && mac_table{RIDX,3}...
     ~=0 && mac_table{RIDX,4} ~= 0,1:size(mac_table,1) ));
fprintf('%d MAC addresses overlap\n',length(rows)); 
mac_raster = cell2mat(mac_table(:,2:4));

