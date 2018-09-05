for /F "tokens=*" %%I in (Config.ini) do set %%I

%engine% %project% %ipadress%:%port% -game -log
pause