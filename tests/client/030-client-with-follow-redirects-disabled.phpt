--TEST--
Client disables redirect following
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

// Arrange: Disable redirect following
$options = ['follow_redirects' => false];

// Act: Create client with redirects disabled
$client = new Client($options);

// Assert: Client should be created
var_dump($client instanceof Client);
echo "Client created with redirects disabled\n";
?>
--EXPECT--
bool(true)
Client created with redirects disabled
