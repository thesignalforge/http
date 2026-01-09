--TEST--
signalforge_http: Response PSR-7 ResponseInterface compliance
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;
use Signalforge\NativeHttp\Stream;

$response = Response::create(200, ['Content-Type' => 'application/json']);

// Status tests
var_dump($response->getStatusCode() === 200);
var_dump($response->getReasonPhrase() === 'OK');

$newResponse = $response->withStatus(404, 'Not Found');
var_dump($newResponse->getStatusCode() === 404);
var_dump($newResponse->getReasonPhrase() === 'Not Found');
var_dump($response->getStatusCode() === 200); // Original unchanged

// Header tests
var_dump($response->hasHeader('Content-Type'));
var_dump($response->hasHeader('content-type')); // Case insensitive
$headers = $response->getHeader('Content-Type');
var_dump(is_array($headers));
var_dump($headers[0] === 'application/json');

// Immutability test
$newResponse2 = $response->withHeader('X-Custom', 'value');
var_dump($newResponse2->hasHeader('X-Custom'));
var_dump(!$response->hasHeader('X-Custom')); // Original unchanged
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)

