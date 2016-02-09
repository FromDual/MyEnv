DROP PROCEDURE IF EXISTS `get_auto_inc`;

DELIMITER //

CREATE DEFINER=`root`@`localhost` PROCEDURE `get_auto_inc` (
  IN db_name VARCHAR(50)
, IN tbl_name VARCHAR(50)
, OUT auto_inc VARCHAR(10)
)
BEGIN
	SET @inc := 0;
	SET @qry = CONCAT("SELECT AUTO_INCREMENT INTO @inc FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = '", db_name, "' AND TABLE_NAME = '", tbl_name, "';");
	PREPARE stmt FROM @qry;
	EXECUTE stmt;
	SET auto_inc = @inc;
END;
//

DELIMITER ;

DROP PROCEDURE IF EXISTS `drop_table`;

DELIMITER //

CREATE DEFINER=`root`@`localhost` PROCEDURE `drop_table` (
  IN db_name VARCHAR(50)
, IN tbl_name VARCHAR(50)
)
BEGIN
	SET @qry = CONCAT('DROP TABLE ', db_name, '.', tbl_name, ';');
	PREPARE stmt FROM @qry;
	EXECUTE stmt;
	DEALLOCATE PREPARE stmt;
END;
//

DELIMITER ;

DROP PROCEDURE IF EXISTS `take_table_offline`;

DELIMITER //

CREATE DEFINER=`root`@`localhost` PROCEDURE `take_table_offline` (
  IN db_name VARCHAR(50)
, IN tbl_name VARCHAR(50)
)
BEGIN
	DECLARE old_name VARCHAR(50);

	SET old_name = CONCAT(tbl_name, '_old');
	SET @qry = CONCAT('RENAME TABLE ', db_name, '.', tbl_name, ' TO ', db_name, '.', old_name, ';');
	PREPARE stmt FROM @qry;
	EXECUTE stmt;

	SET @qry = CONCAT('CREATE TABLE ', db_name, '.', tbl_name, ' LIKE ', db_name, '.', old_name, ';');
	PREPARE stmt FROM @qry;
	EXECUTE stmt;

	CALL get_auto_inc(db_name, old_name, @auto_inc);
	IF @auto_inc IS NOT NULL THEN
		SET @qry = CONCAT('ALTER TABLE ', db_name, '.', tbl_name, ' AUTO_INCREMENT = ', @auto_inc, ';');
		PREPARE stmt FROM @qry;
		EXECUTE stmt;
	END IF;

	DEALLOCATE PREPARE stmt;
END;
//

DELIMITER ;

DROP PROCEDURE IF EXISTS `bring_table_online`;

DELIMITER //

CREATE DEFINER=`root`@`localhost` PROCEDURE `bring_table_online` (
  IN db_name VARCHAR(50)
, IN tbl_name VARCHAR(50)
)
BEGIN
	DECLARE old_name,new_name VARCHAR(50);

	SET old_name = CONCAT(tbl_name, '_old');
	SET new_name = CONCAT(tbl_name, '_new');
	SET @qry = CONCAT('RENAME TABLE ', db_name, '.', tbl_name, ' TO ', db_name, '.', new_name, ';');
	PREPARE stmt FROM @qry;
	EXECUTE stmt;

	SET @qry = CONCAT('RENAME TABLE ', db_name, '.', old_name, ' TO ', db_name, '.', tbl_name, ';');
	PREPARE stmt FROM @qry;
	EXECUTE stmt;

	CALL get_auto_inc(db_name, new_name, @auto_inc);
	IF @auto_inc IS NOT NULL THEN
		SET @qry = CONCAT('ALTER TABLE ', db_name, '.', tbl_name, ' AUTO_INCREMENT = ', @auto_inc, ';');
		PREPARE stmt FROM @qry;
		EXECUTE stmt;
	END IF;

	SET @qry = CONCAT('INSERT INTO ', db_name, '.', tbl_name, ' SELECT * FROM ', db_name, '.', new_name, ';');
	PREPARE stmt FROM @qry;
	EXECUTE stmt;

	DEALLOCATE PREPARE stmt;
	CALL drop_table(db_name, new_name);
END;
//

DELIMITER ;
