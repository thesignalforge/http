--TEST--
signalforge_http: Response PSR-7 MessageInterface withBody() basic replacement and immutability
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\{Response, Stream};

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ARRANGE: Create stream body
$streamBody = Stream::fromString('response body content');

// ACT: Create response with body
$response1 = $response->withBody($streamBody);

// ASSERT: Body replaced, original unchanged
var_dump($response1->getBody()->getContents() === 'response body content');
var_dump($response->getBody()->getSize() === 0);
var_dump($response !== $response1);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
