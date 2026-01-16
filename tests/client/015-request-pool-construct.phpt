--TEST--
HttpRequestPool constructs with client
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
use Signalforge\NativeHttp\HttpRequestPool;

// Arrange: Create a client
$client = new Client();

// Act: Create request pool with default concurrency
$pool = new HttpRequestPool($client);

// Assert: Pool should be created successfully
var_dump($pool instanceof HttpRequestPool);
?>
--EXPECT--
bool(true)
