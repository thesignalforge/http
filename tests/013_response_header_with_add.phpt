--TEST--
signalforge_http: Response PSR-7 MessageInterface withHeader() adds header
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ACT: Add header using withHeader()
$response1 = $response->withHeader('Content-Type', 'application/json');

// ASSERT: Header added, original unchanged
var_dump($response1->hasHeader('Content-Type'));
var_dump(!$response->hasHeader('Content-Type'));
var_dump($response !== $response1);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
