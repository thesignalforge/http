--TEST--
Client constructs with custom pool_size option
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

// Arrange: Configure pool size of 4 threads
$options = ['pool_size' => 4];

// Act: Create client with custom pool size
$client = new Client($options);

// Assert: Client should be created successfully
var_dump($client instanceof Client);
?>
--EXPECT--
bool(true)
