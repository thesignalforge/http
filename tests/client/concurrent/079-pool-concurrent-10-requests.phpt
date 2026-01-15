--TEST--
Client: Execute 10 concurrent requests
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
$client = new Client(['pool_size' => 4, 'timeout' => 30]);
$pool = new HttpRequestPool($client, 10);
$requestFactory = new RequestFactory();

// Add 10 concurrent requests
for ($i = 1; $i <= 10; $i++) {
    $request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/' . $i);
    $pool->add($request);
}

// ACT: Wait for all requests to complete
try {
    $startTime = microtime(true);
    $responses = $pool->wait();
    $duration = microtime(true) - $startTime;

    // ASSERT: Verify all requests completed
    var_dump(count($responses) === 10);
    var_dump($duration < 30); // Should complete faster than sequential
    echo "10 concurrent requests completed successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
10 concurrent requests completed successfully
