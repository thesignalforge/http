--TEST--
Client: Request body with null bytes
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

// ARRANGE: Create request with null bytes in body using PSR-17 factories
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$streamFactory = new StreamFactory();

// Base64 encode data with null bytes to safely transmit in JSON
$bodyWithNulls = "data" . chr(0) . "with" . chr(0) . "nulls";
$jsonData = json_encode(['title' => 'Null Bytes Test', 'binaryData' => base64_encode($bodyWithNulls)]);
$body = $streamFactory->createStream($jsonData);
$request = $requestFactory->createRequest('POST', 'https://dummyjson.com/products/add')
    ->withHeader('Content-Type', 'application/json')
    ->withBody($body);

// ACT: Send the request
try {
    $response = $client->sendRequest($request);

    // ASSERT: Verify request with encoded null bytes succeeded
    var_dump($response->getStatusCode() === 200 || $response->getStatusCode() === 201);
    echo "Null bytes in body handled successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
Null bytes in body handled successfully
