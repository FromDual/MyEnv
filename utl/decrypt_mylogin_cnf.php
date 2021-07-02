#!/usr/bin/php
<?php

/*

http://ocelot.ca/blog/blog/2015/05/21/decrypt-mylogin-cnf/

Decrypt and display a MySQL .mylogin.cnf file.

Uses openSSL libcrypto.so library. Does not use a MySQL library.

*/

$rc = 0;

function mysql_aes_decrypt($val, $ky)
{
	$key = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
	for ( $a = 0; $a < strlen($ky); $a++) {
		$key[$a%16] = chr(ord($key[$a%16]) ^ ord($ky[$a]));
	}
	$mode = MCRYPT_MODE_ECB;
	$enc = MCRYPT_RIJNDAEL_128;
	$dec = @mcrypt_decrypt($enc, $key, $val, $mode, @mcrypt_create_iv( @mcrypt_get_iv_size($enc, $mode), MCRYPT_DEV_URANDOM ) );
	return rtrim($dec, ((ord(substr($dec, strlen($dec) - 1, 1)) >= 0 and ord(substr($dec, strlen($dec)-1, 1)) <= 16) ? chr(ord(substr($dec, strlen($dec)-1, 1))) : null));
}

define('AES_BLOCK_SIZE',  16);

/*
mysql_config_editor set --login-path=oli --user=oli --password
mysql_config_editor print --login-path=oli



typedef struct aes_key_st { unsigned char x[244]; } AES_KEY;


unsigned char cipher_chunk[4096], output_buffer[65536];
int fd, cipher_chunk_length, , i;
char key_in_file[20];
AES_KEY key_for_aes;
*/

$output_buffer = str_repeat("\0", 65536);
$cipher_chunk = str_repeat("\0", 4096);

$output_length = 0;
$key_after_xor = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

if ( $argc < 2 ) {
	return 1;
}

if ( ($fd = fopen($argv[1], 'r')) === false ) {
	return 2;
}

if ( fseek($fd, 4, SEEK_SET) === false ) {
	return 3;
}

if ( ($key_in_file = fread($fd, 20)) === false ) {
	return 4;
}

if ( strlen($key_in_file) != 20 ) {
	return 5;
}

for ( $i = 0; $i < 20; ++$i) {
	$key_after_xor[($i % 16)] = chr(ord($key_after_xor[($i % 16)]) ^ ord($key_in_file[$i]));
}

// for ( $i = 0; $i < 16; ++$i) {
// 	printf("%d: %d %d\n", $i, ord($key_after_xor[$i]), ord($key_after_xor[($i % 16)]) );
// }

while ( ($r = fread($fd, 4)) && (strlen($r) == 4) && (ord($r) > 0) ) {

	$cipher_chunk_length = ord($r);
	// printf("%d\n", $cipher_chunk_length);

	if ( $cipher_chunk_length > 4096 ) {
		return 6;
	}

	if ( (($cipher_chunk = fread($fd, $cipher_chunk_length)) === false) || (strlen($cipher_chunk) != $cipher_chunk_length) ) {
		return 7;
	}
	// printf("%s", $cipher_chunk);

	for ( $i = 0; $i < $cipher_chunk_length; $i += AES_BLOCK_SIZE ) {

		// sudo apt-get install php-mcrypt mcrypt
		// cp /etc/php/mods-available/mcrypt.ini /etc/php/cli/conf.d/
		$dec = trim(mcrypt_decrypt(MCRYPT_RIJNDAEL_128, $key_after_xor, $cipher_chunk, MCRYPT_MODE_ECB));
		printf("%s\n", $dec);
	}
}

exit($rc);

?>
