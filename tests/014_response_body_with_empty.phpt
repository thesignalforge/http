--TEST--
signalforge_http: Response PSR-7 MessageInterface withBody() with empty stream
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\{Response, Stream};

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ARRANGE: Test with empty stream
$emptyStream = Stream::fromString('');

// ACT: Set empty body
$response2 = $response->withBody($emptyStream);

// ASSERT: Empty body set
var_dump($response2->getBody()->getSize() === 0);
var_dump($response2->getBody()->getContents() === '');
?>
--EXPECT--
bool(true)
bool(true)
