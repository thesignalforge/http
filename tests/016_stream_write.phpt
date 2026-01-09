--TEST--
signalforge_http: Stream PSR-7 StreamInterface write() on writable stream
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ARRANGE: Create writable stream from resource
$resource = fopen('php://memory', 'r+');
$writableStream = Stream::fromResource($resource);

// ASSERT: Stream is writable
var_dump($writableStream->isWritable());
var_dump($writableStream->isReadable());

// ACT: Write to stream
$bytesWritten = $writableStream->write('Test Data');

// ASSERT: Write successful
var_dump($bytesWritten === 9);
var_dump($writableStream->getSize() === 9);

// ACT: Rewind and read
$writableStream->rewind();
$readData = $writableStream->read(9);

// ASSERT: Read written data
var_dump($readData === 'Test Data');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
