/**/
Call RxFuncAdd 'SysLoadFuncs', 'REXXUTIL','SysLoadFuncs'
Call SysLoadFuncs
call SysFileDelete 'build_id.c'
call SysFileDelete 'mpu401.lrf'

build_line =  linein('build')
parse var build_line build_ver build_num
call stream 'build', c, close
call SysFileDelete 'build'

call stream build, c, open write
call lineout 'build', build_ver build_num+1
call stream 'build_id.c', c, close

call stream 'mpu401.lrf', c, open write
call lineout 'mpu401.lrf', "option description '@#TBS:2."build_ver"."build_num"#@ Theta Band Software MPU-401 Audio Driver'"
call stream 'mpu401.lrf', c, close

call stream 'build_id.c', c, open write
day = right(date(s), 2);
month = substr(date(s), 5, 2)
if left(month,1) = '0' then month = right(month,1)
if left(day,1) = '0' then day = right(day,1)
call lineout 'build_id.c', '#pragma data_seg ("_initdata","endds");'
call lineout 'build_id.c', 'unsigned short usBuildYear =' left(date(s), 4)';'
call lineout 'build_id.c', 'unsigned short usBuildMonth =' month';'
call lineout 'build_id.c', 'unsigned short usBuildDay =' day';'
call lineout 'build_id.c', 'char szInit[] = "MPU-401 Audio Device Driver 2.'build_ver'.'build_num', Copyright 1999 by Theta Band Software.\n";'
call stream 'build_id.c', c, close

