--TEST--
signalforge_http: Stream PSR-7 StreamInterface getSize() returns correct size
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ARRANGE: Create stream
$stream = Stream::fromString('Hello World');

// ASSERT: Get size
var_dump($stream->getSize() === 11);

// ARRANGE: Create stream from resource
$resource = fopen('php://memory', 'r+');
fwrite($resource, 'Test');
rewind($resource);
$resourceStream = Stream::fromResource($resource);

// ASSERT: Get size from resource stream
var_dump($resourceStream->getSize() === 4);
?>
--EXPECT--
bool(true)
bool(true)
