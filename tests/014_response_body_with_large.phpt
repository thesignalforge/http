--TEST--
signalforge_http: Response PSR-7 MessageInterface withBody() with large content
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\{Response, Stream};

// ARRANGE & ACT: Create response using create() for setup only
$response = Response::create(200);

// ARRANGE: Test with large body
$largeContent = str_repeat('x', 10000);
$largeStream = Stream::fromString($largeContent);

// ACT: Set large body
$response3 = $response->withBody($largeStream);

// ASSERT: Large body set correctly
var_dump($response3->getBody()->getSize() === 10000);
var_dump(strlen($response3->getBody()->getContents()) === 10000);
?>
--EXPECT--
bool(true)
bool(true)
