--TEST--
Client disables SSL verification
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

// Arrange: Disable SSL verification
$options = [
    'verify_peer' => false,
    'verify_host' => false,
];

// Act: Create client with SSL verification disabled
$client = new Client($options);

// Assert: Client should be created
var_dump($client instanceof Client);
echo "Client created with SSL verification disabled\n";
?>
--EXPECT--
bool(true)
Client created with SSL verification disabled
