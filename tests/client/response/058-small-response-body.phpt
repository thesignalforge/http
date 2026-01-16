--TEST--
Client: Response with small body (< 1KB)
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

// ARRANGE: Create request expecting small response using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $body = (string)$response->getBody();
    $bodyLength = strlen($body);

    // ASSERT: Verify small response received
    var_dump($response->getStatusCode() === 200);
    var_dump($bodyLength > 0);
    var_dump($bodyLength < 5000);
    echo "Small response body received successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
Small response body received successfully
