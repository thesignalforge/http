--TEST--
Client: HttpRequestPool::cancel() clears pending requests
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

// Arrange: Create pool and add requests using PSR-17 factory
$client = new Client(['pool_size' => 2, 'timeout' => 10]);
$pool = new HttpRequestPool($client, 10);
$requestFactory = new RequestFactory();
$pool->add($requestFactory->createRequest('GET', 'https://dummyjson.com/products/1'));
$pool->add($requestFactory->createRequest('GET', 'https://dummyjson.com/products/2'));

// Act: Cancel the pool
$pool->cancel();

// Assert: Wait should return empty array
$responses = $pool->wait();
var_dump(count($responses));
echo "Pool cancelled successfully\n";
?>
--EXPECT--
int(0)
Pool cancelled successfully
