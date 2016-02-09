DROP PROCEDURE IF EXISTS kill_sessions;

DELIMITER //

-- CALL kill_sessions('appl', 'localhost', 3);

CREATE DEFINER = 'root'@'localhost' PROCEDURE `kill_sessions` (
  IN user_name VARCHAR(100)
, IN host_name VARCHAR(50)
, IN running_time INT
)
BEGIN
DECLARE done INT DEFAULT FALSE;
DECLARE get_id BIGINT UNSIGNED;

DECLARE cur_all CURSOR FOR
SELECT ID
  FROM INFORMATION_SCHEMA.PROCESSLIST
 WHERE USER = user_name
   AND TIME >= running_time
;

DECLARE cur_host CURSOR FOR
SELECT ID
  FROM INFORMATION_SCHEMA.PROCESSLIST
 WHERE USER = user_name
   AND HOST
  LIKE CONCAT(host_name, '%')
   AND TIME >= running_time
;

DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

IF host_name = '%' THEN
	OPEN cur_all;
		read_loop: LOOP
			FETCH cur_all INTO get_id;
			IF done THEN
				LEAVE read_loop;
			END IF;
			KILL get_id;
		END LOOP;
	CLOSE cur_all;
ELSE
	OPEN cur_host;
		read_loop: LOOP
			FETCH cur_host INTO get_id;
			IF done THEN
				LEAVE read_loop;
			END IF;
			KILL get_id;
		END LOOP;
	CLOSE cur_host;
END IF;

END;
//

DELIMITER ;

DROP PROCEDURE IF EXISTS kill_idle_sessions;

DELIMITER //

-- CALL kill_idle_sessions('appl', 'localhost', 60);

CREATE DEFINER = 'root'@'localhost' PROCEDURE `kill_idle_sessions` (
  IN user_name VARCHAR(100)
, IN host_name VARCHAR(50)
, IN running_time INT
)
BEGIN
DECLARE done INT DEFAULT FALSE;
DECLARE get_id BIGINT UNSIGNED;
DECLARE get_command VARCHAR(50);

DECLARE cur_all CURSOR FOR
SELECT ID, Command
  FROM INFORMATION_SCHEMA.PROCESSLIST
 WHERE USER = user_name
   AND TIME >= running_time
;

DECLARE cur_host CURSOR FOR
SELECT ID, Command
  FROM INFORMATION_SCHEMA.PROCESSLIST
 WHERE USER = user_name
   AND HOST LIKE CONCAT(host_name, '%')
   AND TIME >= running_time
;

DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

IF host_name = '%' THEN
	OPEN cur_all;
		read_loop: LOOP
			FETCH cur_all INTO get_id, get_command;
			IF done THEN
				LEAVE read_loop;
			END IF;
			IF get_command LIKE 'Sleep' THEN
				KILL get_id;
			END IF;
		END LOOP;
	CLOSE cur_all;
ELSE
	OPEN cur_host;
		read_loop: LOOP
			FETCH cur_host INTO get_id, get_command;
			IF done THEN
				LEAVE read_loop;
			END IF;
			IF get_command LIKE 'Sleep' THEN
				KILL get_id;
			END IF;
		END LOOP;
	CLOSE cur_host;
END IF;

END;
//

DELIMITER ;
