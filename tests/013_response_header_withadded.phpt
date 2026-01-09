--TEST--
signalforge_http: Response PSR-7 MessageInterface withAddedHeader() adds multiple values
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ACT: Add multiple values using withAddedHeader()
$response1 = $response->withHeader('X-Custom', 'value1');
$response2 = $response1->withAddedHeader('X-Custom', 'value2');

// ASSERT: Multiple values added
var_dump($response2->getHeader('X-Custom') === ['value1', 'value2']);
var_dump($response2->getHeaderLine('X-Custom') === 'value1,value2');
var_dump($response !== $response2);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
