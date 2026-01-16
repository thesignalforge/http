--TEST--
Client: Response with large body (multiple products)
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

// ARRANGE: Create request for large response using PSR-17 factory
$client = new Client(['timeout' => 60]);
$requestFactory = new RequestFactory();
// Get all products (large response)
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products?limit=100');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $body = (string)$response->getBody();
    $bodyLength = strlen($body);
    $data = json_decode($body, true);

    // ASSERT: Verify large response received
    var_dump($response->getStatusCode() === 200);
    var_dump($bodyLength > 10000);
    echo "Large response body received successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
Large response body received successfully
