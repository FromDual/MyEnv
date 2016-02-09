<?php

/*

	This library contains all functions which are needed for independent
	installer related things.

	answerQuestion($str, $aAllowedKeys, $defaultKey)

*/

// ---------------------------------------------------------------------
function answerQuestion($str, $aAllowedKeys, $defaultKey)
// Test cannot be automatized atm.
// ---------------------------------------------------------------------
{
	while ( true ) {

		// readline always caused crippled output for multi-line output
		// thus we do it our self
		// $input = strtolower(readline($str));
		output($str);
		$input = strtolower(readline(''));

		// on CR do default Key
		if ( $input == '' ) {
			$input = $defaultKey;
		}

		foreach( $aAllowedKeys as $key ) {
			if ( $key == $input ) {
				return $input;
			}
		}
	}
}

?>
