--TEST--
signalforge_http: Response PSR-7 MessageInterface getHeader() and getHeaderLine() return correct values
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
var_dump($response1->getHeader('Content-Type') === ['application/json']);
var_dump($response1->getHeaderLine('Content-Type') === 'application/json');
var_dump($response->getHeader('Content-Type') === []); // Original unchanged
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
