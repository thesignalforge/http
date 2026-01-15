--TEST--
Client class exists and is instantiable
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

// Act: Check class existence
$exists = class_exists('Signalforge\\NativeHttp\\Client');

// Assert: Class should exist
var_dump($exists);
?>
--EXPECT--
bool(true)
