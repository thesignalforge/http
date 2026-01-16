--TEST--
Client: POST request with empty body
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

// ARRANGE: Create POST request with empty body using PSR-17 factories
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$streamFactory = new StreamFactory();

$body = $streamFactory->createStream('');
$request = $requestFactory->createRequest('POST', 'https://dummyjson.com/products/add')
    ->withHeader('Content-Type', 'application/json')
    ->withBody($body);

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $statusCode = $response->getStatusCode();

    // ASSERT: Verify empty body POST works (may return error from API but request should complete)
    var_dump($statusCode === 200 || $statusCode === 201 || $statusCode === 400);
    echo "Empty body POST successful\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
Empty body POST successful
