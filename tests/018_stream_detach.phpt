--TEST--
signalforge_http: Stream PSR-7 StreamInterface detach() returns resource and nullifies stream
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Stream;

// ARRANGE: Create stream from resource
$resource = fopen('php://memory', 'r+');
fwrite($resource, 'Test');
rewind($resource);
$resourceStream = Stream::fromResource($resource);

// ACT: Detach resource
$detached = $resourceStream->detach();

// ASSERT: Resource detached
var_dump(is_resource($detached));
var_dump($resourceStream->getSize() === null);
var_dump($resourceStream->getMetadata() === null);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
