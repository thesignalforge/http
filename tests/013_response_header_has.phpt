--TEST--
signalforge_http: Response PSR-7 MessageInterface hasHeader() case-insensitive behavior
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ACT: Add header using withHeader()
$response1 = $response->withHeader('Content-Type', 'application/json');

// ACT: Test case-insensitive header access
var_dump($response1->hasHeader('Content-Type'));
var_dump($response1->hasHeader('content-type'));
var_dump($response1->hasHeader('CONTENT-TYPE'));
var_dump(!$response->hasHeader('Content-Type')); // Original unchanged
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
