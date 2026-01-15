--TEST--
HttpRequestPool constructs with custom concurrency
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

// Arrange: Create a client and set concurrency
$client = new Client(['pool_size' => 4]);
$concurrency = 20;

// Act: Create request pool with custom concurrency
$pool = new HttpRequestPool($client, $concurrency);

// Assert: Pool should be created successfully
var_dump($pool instanceof HttpRequestPool);
?>
--EXPECT--
bool(true)
