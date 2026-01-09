--TEST--
signalforge_http: Response PSR-7 MessageInterface getHeaders() returns default empty array
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ASSERT: MessageInterface - getHeaders() (default empty)
var_dump($response->getHeaders() === []);
?>
--EXPECT--
bool(true)
