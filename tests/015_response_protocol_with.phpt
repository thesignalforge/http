--TEST--
signalforge_http: Response PSR-7 MessageInterface withProtocolVersion() with different versions
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ACT: Change protocol version to 1.0
$response1 = $response->withProtocolVersion('1.0');

// ASSERT: Protocol version changed, original unchanged
var_dump($response1->getProtocolVersion() === '1.0');
var_dump($response->getProtocolVersion() === '1.1');
var_dump($response !== $response1);

// ACT: Change protocol version to 2.0
$response2 = $response->withProtocolVersion('2.0');

// ASSERT: Protocol version changed to 2.0
var_dump($response2->getProtocolVersion() === '2.0');
var_dump($response->getProtocolVersion() === '1.1');

// ACT: Change protocol version to 3.0
$response3 = $response->withProtocolVersion('3.0');

// ASSERT: Protocol version changed to 3.0
var_dump($response3->getProtocolVersion() === '3.0');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
