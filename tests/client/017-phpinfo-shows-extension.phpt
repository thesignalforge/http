--TEST--
Client: phpinfo() displays extension information
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
// Arrange: Capture phpinfo output
ob_start();
phpinfo(INFO_MODULES);
$info = ob_get_clean();

// Act: Check for extension name and PSR standards info
$has_name = str_contains($info, 'signalforge_http');
$has_status = str_contains($info, 'enabled');
$has_psr = str_contains($info, 'PSR-7') || str_contains($info, 'PSR-18');

// Assert: Extension info should be present
var_dump($has_name);
var_dump($has_status);
var_dump($has_psr);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
