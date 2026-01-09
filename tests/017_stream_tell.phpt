--TEST--
signalforge_http: Stream PSR-7 StreamInterface tell() returns current position
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ARRANGE: Create stream
$stream = Stream::fromString('0123456789');

// ASSERT: Stream is seekable
var_dump($stream->isSeekable());

// ACT: Get initial position
var_dump($stream->tell() === 0);
?>
--EXPECT--
bool(true)
bool(true)
