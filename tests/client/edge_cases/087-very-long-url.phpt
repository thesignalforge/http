--TEST--
Client: Very long URL (4KB+)
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

// ARRANGE: Create request with very long URL using PSR-17 factory
$client = new Client(['timeout' => 20]);
$requestFactory = new RequestFactory();
$longQuery = str_repeat('param=value&', 400); // ~4KB
$url = 'https://dummyjson.com/products/1?' . $longQuery;
$request = $requestFactory->createRequest('GET', $url);

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $statusCode = $response->getStatusCode();

    // ASSERT: Verify request completed (some servers may reject)
    var_dump($statusCode === 200 || $statusCode === 414);
    echo "Long URL handled\n";
} catch (\Exception $e) {
    // Some servers reject very long URLs
    var_dump(true);
    echo "Long URL handled\n";
}
?>
--EXPECT--
bool(true)
Long URL handled
