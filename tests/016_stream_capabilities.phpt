--TEST--
signalforge_http: Stream PSR-7 StreamInterface isReadable() and isWritable() flags
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ARRANGE: Create readable stream
$readableStream = Stream::fromString('Hello World');

// ASSERT: Readable stream capabilities
var_dump($readableStream->isReadable());
var_dump(!$readableStream->isWritable());

// ARRANGE: Create writable stream from resource
$resource = fopen('php://memory', 'r+');
$writableStream = Stream::fromResource($resource);

// ASSERT: Writable stream capabilities
var_dump($writableStream->isWritable());
var_dump($writableStream->isReadable());
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
