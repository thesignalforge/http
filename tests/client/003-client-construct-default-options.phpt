--TEST--
Client constructs with default options
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
use Signalforge\NativeHttp\Client;

// Arrange: (no setup needed)

// Act: Create client with default options
$client = new Client();

// Assert: Client should be created successfully
var_dump($client instanceof Client);
?>
--EXPECT--
bool(true)
