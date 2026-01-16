--TEST--
Client constructs with SSL verification options
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

// Arrange: Configure SSL verification
$options = [
    'verify_peer' => true,
    'verify_host' => true,
];

// Act: Create client with SSL options
$client = new Client($options);

// Assert: Client should be created successfully
var_dump($client instanceof Client);
?>
--EXPECT--
bool(true)
