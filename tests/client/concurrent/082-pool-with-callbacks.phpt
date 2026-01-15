--TEST--
Client: Concurrent requests with success/error callbacks
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

// ARRANGE: Create client and pool with callbacks using PSR-17 factory
$client = new Client(['pool_size' => 4, 'timeout' => 30]);
$pool = new HttpRequestPool($client, 10);
$requestFactory = new RequestFactory();

$successCount = 0;
$errorCount = 0;

// Add requests with callbacks
for ($i = 1; $i <= 8; $i++) {
    $request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/' . $i);
    $pool->add(
        $request,
        function($response) use (&$successCount) {
            $successCount++;
        },
        function($error) use (&$errorCount) {
            $errorCount++;
        }
    );
}

// Add a request that will return 500
$failRequest = $requestFactory->createRequest('GET', 'https://dummyjson.com/http/500');
$pool->add(
    $failRequest,
    function($response) use (&$successCount) {
        $successCount++;
    },
    function($error) use (&$errorCount) {
        $errorCount++;
    }
);

// ACT: Wait for all requests to complete
try {
    $responses = $pool->wait();

    // ASSERT: Verify callbacks were executed
    var_dump($successCount >= 8);
    var_dump(count($responses) === 9);
    echo "Concurrent requests with callbacks completed successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
Concurrent requests with callbacks completed successfully
