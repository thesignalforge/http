--TEST--
signalforge_http: Stream PSR-7 StreamInterface getMetadata($key) returns specific key or null
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

// ACT: Get specific metadata key
$mode = $resourceStream->getMetadata('mode');
var_dump(is_string($mode));
var_dump($mode === 'w+b');

// ACT: Get non-existent metadata key
$nonExistent = $resourceStream->getMetadata('non_existent_key');
var_dump($nonExistent === null);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
