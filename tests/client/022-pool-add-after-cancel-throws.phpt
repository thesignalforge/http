--TEST--
Client: HttpRequestPool::add() throws exception after cancel
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
use Signalforge\NativeHttp\RequestFactory;

// Arrange: Create pool and cancel it using PSR-17 factory
$client = new Client(['pool_size' => 2, 'timeout' => 10]);
$pool = new HttpRequestPool($client, 10);
$requestFactory = new RequestFactory();
$pool->cancel();

// Act & Assert: Adding request should throw exception
try {
    $pool->add($requestFactory->createRequest('GET', 'https://dummyjson.com/products/1'));
    echo "FAIL: Expected exception not thrown\n";
} catch (Exception $e) {
    echo "Exception caught: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
Exception caught: Cannot add requests to a cancelled pool
