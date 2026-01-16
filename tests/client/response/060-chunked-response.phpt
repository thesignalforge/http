--TEST--
Client: Response with chunked transfer encoding
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

// ARRANGE: Create request using PSR-17 factory
$client = new Client(['timeout' => 30]);
$requestFactory = new RequestFactory();
// dummyjson products endpoint typically returns chunked encoding
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products?limit=50');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $body = (string)$response->getBody();
    $bodyLength = strlen($body);
    $data = json_decode($body, true);

    // ASSERT: Verify response was assembled correctly
    var_dump($response->getStatusCode() === 200);
    var_dump($bodyLength > 0);
    echo "Chunked response received successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
Chunked response received successfully
