--TEST--
Client constructs with custom timeout options
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

// Arrange: Configure timeouts
$options = [
    'connect_timeout' => 5,
    'timeout' => 15,
];

// Act: Create client with custom timeouts
$client = new Client($options);

// Assert: Client should be created successfully
var_dump($client instanceof Client);
?>
--EXPECT--
bool(true)
