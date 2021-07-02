SET user=root
SET password=""
SET host=127.0.0.1
SET port=3306
SET database=test

SET PATH=%PATH%;C:\Program Files\MySQL\MySQL Server 5.7\bin\

@echo off

:while

  mysql --user=%user% --password=%password% --host=%host% --port=%port% %database% -e "INSERT INTO test VALUES (NULL, CONCAT('Test data insert from client on ', @@hostname), CURRENT_TIMESTAMP());" 2>nul
  set /p =.<nul
GOTO while

