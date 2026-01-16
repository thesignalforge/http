--TEST--
Extension loads successfully
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

// Act: Check if extension is loaded
$loaded = extension_loaded('signalforge_http');

// Assert: Extension should be loaded
var_dump($loaded);
?>
--EXPECT--
bool(true)
