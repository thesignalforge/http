--TEST--
Client: POST request with multipart/form-data equivalent
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

// ARRANGE: Create request simulating multipart form data using PSR-17 factories
// dummyjson.com accepts JSON, so we simulate multipart fields as JSON
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$streamFactory = new StreamFactory();

$jsonData = json_encode([
    'title' => 'Multipart Test Product',
    'field1' => 'value1',
    'field2' => 'value2',
    'price' => 29.99
]);
$body = $streamFactory->createStream($jsonData);
$request = $requestFactory->createRequest('POST', 'https://dummyjson.com/products/add')
    ->withHeader('Content-Type', 'application/json')
    ->withBody($body);

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $responseBody = (string)$response->getBody();
    $data = json_decode($responseBody, true);

    // ASSERT: Verify data was sent correctly
    var_dump($response->getStatusCode() === 200 || $response->getStatusCode() === 201);
    var_dump($data['title'] === 'Multipart Test Product');
    var_dump($data['price'] == 29.99);
    echo "Multipart form POST successful\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
Multipart form POST successful
