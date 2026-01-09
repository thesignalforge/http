--TEST--
signalforge_http: Stream PSR-7 StreamInterface getMetadata() returns array
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ARRANGE: Create stream
$stream = Stream::fromString('Hello World');

// ASSERT: Get metadata
$metadata = $stream->getMetadata();
var_dump(is_array($metadata));
var_dump(isset($metadata['mode']));
?>
--EXPECT--
bool(true)
bool(true)
