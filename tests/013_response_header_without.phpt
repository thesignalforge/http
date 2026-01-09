--TEST--
signalforge_http: Response PSR-7 MessageInterface withoutHeader() removes header
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ACT: Add header and then remove it
$response1 = $response->withHeader('X-Custom', 'value');
$response2 = $response1->withoutHeader('X-Custom');

// ASSERT: Header removed
var_dump(!$response2->hasHeader('X-Custom'));
var_dump($response1->hasHeader('X-Custom')); // Original unchanged
var_dump($response1 !== $response2);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
