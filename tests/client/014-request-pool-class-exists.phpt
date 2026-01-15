--TEST--
HttpRequestPool class exists
--EXTENSIONS--
signalforge_http
--SKIPIF--
<?php
if (!class_exists('Signalforge\NativeHttp\Client')) {
    die('skip PSR-18 client not available (requires libcurl)');
}
?>
--FILE--
<?php
// Arrange: (no setup needed)

// Act: Check if HttpRequestPool class exists
$exists = class_exists('Signalforge\\NativeHttp\\HttpRequestPool');

// Assert: Class should exist
var_dump($exists);
?>
--EXPECT--
bool(true)
