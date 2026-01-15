--TEST--
Client: HttpRequestPool::add() accepts error callback
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

// Arrange: Create client, pool, request, and callbacks using PSR-17 factory
$client = new Client(['pool_size' => 2, 'timeout' => 10]);
$pool = new HttpRequestPool($client, 10);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');
$successCallback = function($response) {};
$errorCallback = function($error) { echo "Error callback\n"; };

// Act: Add request with both callbacks
$pool->add($request, $successCallback, $errorCallback);

// Assert: Operation should complete without error
echo "Request with both callbacks added successfully\n";
?>
--EXPECT--
Request with both callbacks added successfully
