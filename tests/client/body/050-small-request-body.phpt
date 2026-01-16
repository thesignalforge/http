--TEST--
Client: POST request with small body (< 1KB)
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

// ARRANGE: Create request with small body using PSR-17 factories
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$streamFactory = new StreamFactory();

$jsonData = json_encode(['title' => 'Small Body Test', 'description' => str_repeat('A', 256)]);
$body = $streamFactory->createStream($jsonData);
$request = $requestFactory->createRequest('POST', 'https://dummyjson.com/products/add')
    ->withHeader('Content-Type', 'application/json')
    ->withBody($body);

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $responseBody = (string)$response->getBody();
    $data = json_decode($responseBody, true);

    // ASSERT: Verify small body was sent correctly
    var_dump($response->getStatusCode() === 200 || $response->getStatusCode() === 201);
    var_dump(isset($data['id']));
    echo "Small body POST successful\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
Small body POST successful
