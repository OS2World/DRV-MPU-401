/**/
'@echo off'
call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
call SysLoadFuncs
parse upper value value("PATH", , "OS2ENVIRONMENT") with "\OS2\SYSTEM" -2 bdrive +2
config = translate(bdrive || '\config.sys')
parse source . . location
installpath = filespec('D', location) || filespec('P', location)
if right(installpath,1) = '\' then installpath = left(installpath, length(installpath)-1)
temp = translate(value('MMBASE',,'OS2ENVIRONMENT'))
if temp = "" then do
   say "Error: MMPM/2 is NOT INSTALLED!"
   say "Please install MMPM/2 first, and then install this driver."
   exit
end
mmpm2path = left(temp, pos(';', temp) - 1)
if right(mmpm2path,1) \= '\' then mmpm2path = mmpm2path'\'
updated = 0
if stream(mmpm2path || 'vmpu401.sys', 'c', 'query exists') \= '' then
   call update_mmpack
else if stream(mmpm2path || 'mpu401.sys', 'c', 'query exists') \= '' then
   call update_ibm
if stream(mmpm2path || 'cwmpu401.sys', 'c', 'query exists') \= '' then
   call update_crystal
if updated = 0 then
   call install

say "Installation completed"
'pause'
exit

install:
   say "No existing driver to update, running normal installation."
   call setlocal
   call directory(installpath)
   'minstall'
   call endlocal
   call SysCreateObject "WPProgram", 'MPU-401 Info', '<MMPM2_FOLDER>', 'EXENAME=VIEW.EXE;STARTUPDIR='||mmpm2path||';PARAMETERS=MPU401.INF', 'U'
   return

update_mmpack:
   say "Updating the currently installed MM Pack MPU-401 driver"
   'copy' mmpm2path'mpu401.sys' mmpm2path'mpu401.bak >/dev/nul'
   'copy' mmpm2path'vmpu401.sys' mmpm2path'vmpu401.bak >/dev/nul'
   'copy' installpath'\mpu401.sys' mmpm2path'mpu401.sys >/dev/nul'
   'copy' installpath'\vmpu401.sys' mmpm2path'vmpu401.sys >/dev/nul'
   'copy' installpath'\mpu401.inf' mmpm2path'mpu401.inf >/dev/nul'
   call SysCreateObject "WPProgram", 'MPU-401 Info', '<MMPM2_FOLDER>', 'EXENAME=VIEW.EXE;STARTUPDIR='||mmpm2path||';PARAMETERS=MPU401.INF', 'U'
   updated = 1
   return

update_ibm:
   say "Installing the MM Pack MPU-401 driver over the IBM MPU-401 driver"
   'copy' mmpm2path'mpu401.sys' mmpm2path'mpu401.bak >/dev/nul'
   'copy' installpath'\mpu401.sys' mmpm2path'mpu401.sys >/dev/nul'
   'copy' installpath'\vmpu401.sys' mmpm2path'vmpu401.sys >/dev/nul'
   'copy' installpath'\mpu401.inf' mmpm2path'mpu401.inf >/dev/nul'
   call SysCreateObject "WPProgram", 'MPU-401 Info', '<MMPM2_FOLDER>', 'EXENAME=VIEW.EXE;STARTUPDIR='||mmpm2path||';PARAMETERS=MPU401.INF', 'U'
   call add_vmpu401
   updated = 1
   return

update_crystal:
   say "Installing the MM Pack MPU-401 driver over the Crystal Semiconductor MPU-401"
   say "driver"
   'copy' mmpm2path'cwmpu401.sys' mmpm2path'cwmpu401.bak >/dev/nul'
   'copy' installpath'\mpu401.sys' mmpm2path'cwmpu401.sys >/dev/nul'
   'copy' installpath'\vmpu401.sys' mmpm2path'vmpu401.sys >/dev/nul'
   'copy' installpath'\mpu401.inf' mmpm2path'mpu401.inf >/dev/nul'
   call SysCreateObject "WPProgram", 'MPU-401 Info', '<MMPM2_FOLDER>', 'EXENAME=VIEW.EXE;STARTUPDIR='||mmpm2path||';PARAMETERS=MPU401.INF', 'U'
   call add_vmpu401
   updated = 1
   return

add_vmpu401:
   call find_driver 'MPU401.SYS'
   num_installed = drivers.0
   call find_driver 'CWMPU401.SYS'
   num_installed = num_installed + drivers.0
   if num_installed > 1 then do
      say "Warning: Multiple MPU-401 device drivers installed. This program will"
      say "not update CONFIG.SYS. You will need to update CONFIG.SYS manually."
      say "Please contact Theta Band Software support if you need assistance."
      return
   end

   driver_name = find_parameter('MPU401.SYS /N')
   if driver_name = "" then
      driver_name = find_parameter('CWMPU401.SYS /N')
   driver_name = substr(driver_name, 4);
   call find_driver 'VMPU401.SYS'
   if drivers.0 = 0 then do
      say "Updating" config
      backup = bdrive || '\config.bak'
      'copy' config backup '>/dev/nul'
      rc = stream(backup,'c','open read')
      if (rc \= 'READY:') then
        call errorexit 'Could not open "' || backup || '" (rc=' || rc || ')'
      newconfig = SysTempFileName(bdrive || '\config.???')
      if newconfig = "" then
        call errorexit 'Could not create temporary config.sys (rc=' || rc || ')'
      do until lines(config) = 0
         parse value linein(config) with line
         call lineout newconfig, line
      end
      call lineout newconfig, 'DEVICE=' || mmpm2path || 'VMPU401.SYS ' || driver_name
      call lineout newconfig, ''
      call stream backup, 'c', 'close'
      call stream newconfig, 'c', 'close'
      call stream config, 'c', 'close'
      'copy' newconfig config '>/dev/nul'
   end
   return

/* Determines whether a particular driver is loaded in CONFIG.SYS or not */
find_driver:
   parse upper arg driver
   rc = stream(config,'c','open read')
   if (rc \= 'READY:') then
     call errorexit 'Could not open "' || config || '" (rc=' || rc || ')'

   drivers.0 = 0
   count = 1
   do while lines(config)
     configline = linein(config)
     parse upper var configline editline
     if left(editline, 6) = 'DEVICE' then do
         equal = pos('=', editline);
         first = equal+1;
         do while substr(editline, first, 1) = ' '
            first = first + 1
         end
         last = first;
         do while last <= length(editline)
            if substr(editline, last, 1) == ' ' then leave
            last = last + 1
         end
         last = last-1;
         filename = substr(editline, first, last-first+1)
         drive = filespec('D', filename);
         if drive = "" then drive = bdrive;
         path = filespec('P', filename);
         if path = "" then path = "\";
         name = filespec('N', filename);
         drive_info = SysDriveInfo(drive);
         if drive_info = "" then iterate;
         filename = drive || path || name;
         if SysFileTree(filename, stem, 'F') \= 0 then iterate;
         if stem.0 = 0 then iterate;
         if name = driver then do
            drivers.count = filename
            drivers.0 = count;
            count = count+1
         end
       end
   end

   call stream config, 'c', 'close'
   return

/* Determines whether a particular driver is loaded in CONFIG.SYS or not */
find_parameter:
   parse upper arg driver parm
   rc = stream(config,'c','open read')
   if (rc \= 'READY:') then
     call errorexit 'Could not open "' || config || '" (rc=' || rc || ')'

   count = 1
   do while lines(config)
     configline = linein(config)
     parse upper var configline editline
     if left(editline, 6) = 'DEVICE' then do
         equal = pos('=', editline);
         first = equal+1;
         do while substr(editline, first, 1) = ' '
            first = first + 1
         end
         last = first;
         do while last <= length(editline)
            if substr(editline, last, 1) == ' ' then leave
            last = last + 1
         end
         last = last-1;
         filename = substr(editline, first, last-first+1)
         drive = filespec('D', filename);
         if drive = "" then drive = bdrive;
         path = filespec('P', filename);
         if path = "" then path = "\";
         name = filespec('N', filename);
         drive_info = SysDriveInfo(drive);
         if drive_info = "" then iterate;
         filename = drive || path || name;
         if SysFileTree(filename, stem, 'F') \= 0 then iterate;
         if stem.0 = 0 then iterate;
         if name = driver then do
            call stream config, 'c', 'close'
            l = length(parm)
            do i = 1 to words(editline)
               if left(word(editline, i), l) = parm then
                  return word(editline, i)
            end
            return ''
         end
       end
   end

   call stream config, 'c', 'close'
   return ''

errorexit:
  parse arg msg
  say
  say ''
  say '  Error: ' || msg
  exit

