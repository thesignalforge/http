--TEST--
Client: PUT request with body
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
use Signalforge\NativeHttp\StreamFactory;

// ARRANGE: Create PUT request using PSR-17 factories
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$streamFactory = new StreamFactory();

$jsonData = json_encode(['title' => 'Updated Product', 'price' => 199.99]);
$body = $streamFactory->createStream($jsonData);
$request = $requestFactory->createRequest('PUT', 'https://dummyjson.com/products/1')
    ->withHeader('Content-Type', 'application/json')
    ->withBody($body);

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $responseBody = (string)$response->getBody();
    $data = json_decode($responseBody, true);

    // ASSERT: Verify PUT was successful
    var_dump($response->getStatusCode() === 200);
    var_dump($data['title'] === 'Updated Product');
    var_dump($data['price'] == 199.99);
    echo "PUT request successful\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
PUT request successful
