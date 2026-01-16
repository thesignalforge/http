--TEST--
Client: Mixed GET and POST requests concurrently
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
use Signalforge\NativeHttp\StreamFactory;

// ARRANGE: Create client and pool using PSR-17 factories
$client = new Client(['pool_size' => 4, 'timeout' => 30]);
$pool = new HttpRequestPool($client, 20);
$requestFactory = new RequestFactory();
$streamFactory = new StreamFactory();

// Add mixed GET and POST requests
for ($i = 0; $i < 10; $i++) {
    $productId = ($i % 30) + 1;
    $getRequest = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/' . $productId);
    $pool->add($getRequest);

    $postData = json_encode(['title' => 'Product ' . $i, 'price' => 9.99 + $i]);
    $body = $streamFactory->createStream($postData);
    $postRequest = $requestFactory->createRequest('POST', 'https://dummyjson.com/products/add')
        ->withHeader('Content-Type', 'application/json')
        ->withBody($body);
    $pool->add($postRequest);
}

// ACT: Wait for all requests to complete
try {
    $responses = $pool->wait();

    // ASSERT: Verify all mixed requests completed
    var_dump(count($responses) === 20);
    echo "Mixed concurrent requests completed successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
Mixed concurrent requests completed successfully
