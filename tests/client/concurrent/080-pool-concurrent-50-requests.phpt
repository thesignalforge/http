--TEST--
Client: Execute 50 concurrent requests
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

// ARRANGE: Create client and pool using PSR-17 factory
$client = new Client(['pool_size' => 8, 'timeout' => 60]);
$pool = new HttpRequestPool($client, 50);
$requestFactory = new RequestFactory();

// Add 50 concurrent requests (cycling through product IDs)
for ($i = 0; $i < 50; $i++) {
    $productId = ($i % 30) + 1; // dummyjson has 30 products
    $request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/' . $productId);
    $pool->add($request);
}

// ACT: Wait for all requests to complete
try {
    $startTime = microtime(true);
    $responses = $pool->wait();
    $duration = microtime(true) - $startTime;

    // ASSERT: Verify all requests completed
    var_dump(count($responses) === 50);
    var_dump($duration < 60);
    echo "50 concurrent requests completed successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
50 concurrent requests completed successfully
