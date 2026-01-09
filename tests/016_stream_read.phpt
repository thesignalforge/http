--TEST--
signalforge_http: Stream PSR-7 StreamInterface read() and getContents() on readable stream
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ARRANGE: Create readable stream
$stream = Stream::fromString('Hello World');

// ASSERT: Stream is readable
var_dump($stream->isReadable());

// ACT: Read from stream
$data1 = $stream->read(5);

// ASSERT: Read correct data
var_dump($data1 === 'Hello');
var_dump($stream->tell() === 5);

// ACT: Read remaining data
$data2 = $stream->getContents();

// ASSERT: Read remaining data
var_dump($data2 === ' World');
var_dump($stream->eof());
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
