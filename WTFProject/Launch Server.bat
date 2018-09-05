for /F "tokens=*" %%I in (Config.ini) do set %%I

%engine% %project% %map% -server -log -port=%port%
pause