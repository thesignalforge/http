--TEST--
Client: POST request with JSON body
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

// ARRANGE: Create POST request with JSON body using PSR-17 factories
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$streamFactory = new StreamFactory();

$jsonData = json_encode(['title' => 'Test Product', 'price' => 99.99, 'category' => 'electronics']);
$body = $streamFactory->createStream($jsonData);
$request = $requestFactory->createRequest('POST', 'https://dummyjson.com/products/add')
    ->withHeader('Content-Type', 'application/json')
    ->withBody($body);

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $responseBody = (string)$response->getBody();
    $data = json_decode($responseBody, true);

    // ASSERT: Verify JSON was sent and received correctly
    var_dump($response->getStatusCode() === 200 || $response->getStatusCode() === 201);
    var_dump($data['title'] === 'Test Product');
    var_dump($data['price'] == 99.99);
    var_dump(isset($data['id']));
    echo "JSON POST successful\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
JSON POST successful
