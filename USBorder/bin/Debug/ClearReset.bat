echo off
echo.
echo. Wait
echo.
echo. After this reboot a second start of this program is necessary !
echo.
uvd.exe /remove_by_pid 08bb;29c0
uvd.exe /remove_by_pid 0d8c;0013
uvd.exe /remove_by_pid 03eb;2fde
uvd.exe /remove_by_pid 0805;1402
uvd.exe /remove_by_pid 0805;1404
uvd.exe /remove_by_pid 09FB;6001
uvd.exe /remove_by_pid 0403;6001
uvd.exe /remove_by_pid 03eb;2402
uvd.exe /remove_by_pid 03eb;2fe4
echo.
echo. De the slider names are added. 
echo. Reboot now
echo.
shutdown /r /t 9

