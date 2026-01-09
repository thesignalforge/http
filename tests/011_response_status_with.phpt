--TEST--
signalforge_http: Response PSR-7 ResponseInterface withStatus() immutability
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ACT: Test withStatus() immutability
$newResponse = $response->withStatus(404, 'Not Found');

// ASSERT: Original unchanged, new has different status
var_dump($newResponse->getStatusCode() === 404);
var_dump($newResponse->getReasonPhrase() === 'Not Found');
var_dump($response->getStatusCode() === 200);
var_dump($response->getReasonPhrase() === 'OK');
var_dump($response !== $newResponse);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
