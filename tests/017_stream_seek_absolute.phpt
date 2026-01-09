--TEST--
signalforge_http: Stream PSR-7 StreamInterface seek() with absolute position
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ARRANGE: Create stream
$stream = Stream::fromString('0123456789');

// ACT: Seek to position 5
$stream->seek(5);

// ASSERT: Position updated
var_dump($stream->tell() === 5);
var_dump($stream->read(1) === '5');
?>
--EXPECT--
bool(true)
bool(true)
