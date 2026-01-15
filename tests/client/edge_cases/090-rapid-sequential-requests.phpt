--TEST--
Client: Rapid sequential requests (connection reuse)
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
use Signalforge\NativeHttp\RequestFactory;

// ARRANGE: Create client and prepare requests using PSR-17 factory
$client = new Client(['pool_size' => 2, 'timeout' => 30]);
$requestFactory = new RequestFactory();
$successCount = 0;

// ACT: Send 20 rapid sequential requests
try {
    for ($i = 1; $i <= 20; $i++) {
        $productId = (($i - 1) % 30) + 1; // dummyjson has 30 products
        $request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/' . $productId);
        $response = $client->sendRequest($request);
        if ($response->getStatusCode() === 200) {
            $successCount++;
        }
    }

    // ASSERT: Verify all requests succeeded
    var_dump($successCount === 20);
    echo "Rapid sequential requests completed successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
Rapid sequential requests completed successfully
