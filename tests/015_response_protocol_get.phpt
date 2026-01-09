--TEST--
signalforge_http: Response PSR-7 MessageInterface getProtocolVersion() returns default '1.1'
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ASSERT: MessageInterface - getProtocolVersion() (default)
var_dump($response->getProtocolVersion() === '1.1');
?>
--EXPECT--
bool(true)
