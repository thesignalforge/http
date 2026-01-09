--TEST--
signalforge_http: Response PSR-7 ResponseInterface withStatus() custom reason phrase
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ACT: Test withStatus() with custom reason phrase
$customResponse = Response::create(200)->withStatus(418, "I'm a teapot");

// ASSERT: Custom reason phrase preserved
var_dump($customResponse->getStatusCode() === 418);
var_dump($customResponse->getReasonPhrase() === "I'm a teapot");
var_dump($response->getStatusCode() === 200); // Original unchanged
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
