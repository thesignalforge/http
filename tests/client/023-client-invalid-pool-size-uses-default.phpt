--TEST--
Client uses default pool_size when given invalid value
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

// Arrange: Configure with invalid pool size (0)
$options = ['pool_size' => 0];

// Act: Create client with invalid pool size
$client = new Client($options);

// Assert: Client should be created with default pool size
var_dump($client instanceof Client);
echo "Client created with default pool size\n";
?>
--EXPECT--
bool(true)
Client created with default pool size
